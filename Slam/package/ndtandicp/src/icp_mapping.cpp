/*
 * Copyright 2015-2019 Autoware Foundation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 Localization and mapping program using Normal Distributions Transform

 Yuki KITSUKAWA
 */

#include "icp_mapping.h"

icp_mapping::icp_mapping() 
{

  points_sub_ = nh_.subscribe("/os1_points", 100000, &icp_mapping::points_callback,this);
  icp_map_pub_ = nh_.advertise<sensor_msgs::PointCloud2>("/icp_map", 1000);
  current_pose_pub_ = nh_.advertise<geometry_msgs::PoseStamped>("/current_pose", 1000);

  // Default values
  nh_.param("max_iter", max_iter_, 30);
  nh_.param("icp_corres_", icp_corres_, 30.0);
  nh_.param("icp_eucli_", icp_eucli_, 0.01);
  nh_.param("trans_eps", trans_eps_, 0.01);
  nh_.param("voxel_leaf_size", voxel_leaf_size_, 2.0);
  nh_.param("scan_rate", scan_rate_, 10.0);
  nh_.param("min_scan_range", min_scan_range_, 5.0);
  nh_.param("max_scan_range", max_scan_range_, 200.0);
  nh_.param("use_imu", use_imu_, false);  // 不使用imu

  initial_scan_loaded = 0;       // 为载入地图
  min_add_scan_shift_ = 1.0;     // 控制关键帧（previous帧）的选取

  _tf_x=0.0, _tf_y=0.0, _tf_z=0.0, _tf_roll=0.0, _tf_pitch=0.0, _tf_yaw=0.0;

  std::cout << "incremental_voxel_update: " << _incremental_voxel_update << std::endl;
  std::cout << "(tf_x,tf_y,tf_z,tf_roll,tf_pitch,tf_yaw): (" << _tf_x << ", " << _tf_y << ", " << _tf_z << ", "
            << _tf_roll << ", " << _tf_pitch << ", " << _tf_yaw << ")" << std::endl;

  Eigen::Translation3f tl_btol(_tf_x, _tf_y, _tf_z);                 // tl: translation
  Eigen::AngleAxisf rot_x_btol(_tf_roll, Eigen::Vector3f::UnitX());  // rot: rotation
  Eigen::AngleAxisf rot_y_btol(_tf_pitch, Eigen::Vector3f::UnitY());
  Eigen::AngleAxisf rot_z_btol(_tf_yaw, Eigen::Vector3f::UnitZ());
  tf_btol_ = (tl_btol * rot_z_btol * rot_y_btol * rot_x_btol).matrix();   // base to laser  初始是旋转=0,平移=0
  tf_ltob_ = tf_btol_.inverse();

  map_.header.frame_id = "map";

  // 当前pose和之前的pose
  current_pose_.x = current_pose_.y = current_pose_.z = 0.0;current_pose_.roll = current_pose_.pitch = current_pose_.yaw = 0.0;
  previous_pose_.x = previous_pose_.y = previous_pose_.z = 0.0;previous_pose_.roll = previous_pose_.pitch = previous_pose_.yaw = 0.0;

  voxel_grid_filter_.setLeafSize(voxel_leaf_size_, voxel_leaf_size_, voxel_leaf_size_);  

  icp.setMaxCorrespondenceDistance(icp_corres_);//当两个点云相距较远时候，距离值要变大，所以一开始需要粗配准。
  icp.setTransformationEpsilon(trans_eps_);//svd奇异值分解，对icp时间影响不大
  icp.setEuclideanFitnessEpsilon(icp_eucli_);//前后两次误差大小，当误差值小于这个值停止迭代
  icp.setMaximumIterations(max_iter_);//最大迭代次数


  is_first_map_ = true;    // 是初始地图

  std::cout << "icp_corres_: " << icp_corres_ << std::endl;
  std::cout << "icp_eucli_: " << icp_eucli_ << std::endl;
  std::cout << "trans_epsilon: " << trans_eps_ << std::endl;
  std::cout << "max_iter: " << max_iter_ << std::endl;
  std::cout << "voxel_leaf_size: " << voxel_leaf_size_ << std::endl;
  std::cout << "min_scan_range: " << min_scan_range_ << std::endl;
  std::cout << "max_scan_range: " << max_scan_range_ << std::endl;
  std::cout << "min_add_scan_shift: " << min_add_scan_shift_ << std::endl;
}; 

icp_mapping::~icp_mapping(){}; 

void icp_mapping::points_callback(const sensor_msgs::PointCloud2::ConstPtr& input)
{
  pcl::PointCloud<pcl::PointXYZI> tmp, scan;
  pcl::PointCloud<pcl::PointXYZI>::Ptr filtered_scan_ptr(new pcl::PointCloud<pcl::PointXYZI>());
  pcl::PointCloud<pcl::PointXYZI>::Ptr transformed_scan_ptr(new pcl::PointCloud<pcl::PointXYZI>());
  tf::Quaternion q;

  Eigen::Matrix4f t_localizer(Eigen::Matrix4f::Identity());
  Eigen::Matrix4f t_base_link(Eigen::Matrix4f::Identity());
  //static tf::TransformBroadcaster br;
  tf::Transform transform;

  // tmp 裁剪后 得到scan
  pcl::fromROSMsg(*input, tmp);  
  double r;
  Eigen::Vector3d point_pos;
  pcl::PointXYZI p;
  for (pcl::PointCloud<pcl::PointXYZI>::const_iterator item = tmp.begin(); item != tmp.end(); item++)
  {
    if(use_imu_){  // 使用imu预测pose
      // deskew(TODO:inplement of predicting pose by imu)
      point_pos.x() = (double)item->x;
      point_pos.y() = (double)item->y;
      point_pos.z() = (double)item->z;
      double s = scan_rate_ * (double(item->intensity) - int(item->intensity));

      point_pos.x() -= s * current_pose_msg_.pose.position.x;//current_pose_imu_
      point_pos.y() -= s * current_pose_msg_.pose.position.y;
      point_pos.z() -= s * current_pose_msg_.pose.position.z;

      Eigen::Quaterniond start_quat, end_quat, mid_quat;
      mid_quat.setIdentity();
      end_quat = Eigen::Quaterniond(
        current_pose_msg_.pose.orientation.w,
        current_pose_msg_.pose.orientation.x,
        current_pose_msg_.pose.orientation.y,
        current_pose_msg_.pose.orientation.z);
      start_quat = mid_quat.slerp(s, end_quat);

      point_pos = start_quat.conjugate() * start_quat * point_pos;

      point_pos.x() += current_pose_msg_.pose.position.x;
      point_pos.y() += current_pose_msg_.pose.position.y;
      point_pos.z() += current_pose_msg_.pose.position.z;

      p.x = point_pos.x();
      p.y = point_pos.y();
      p.z = point_pos.z();
    }
    else{ // 不使用imu
      p.x = (double)item->x;
      p.y = (double)item->y;
      p.z = (double)item->z;
    }
    p.intensity = (double)item->intensity;
  
    // minmax 裁剪点云
    r = sqrt(pow(p.x, 2.0) + pow(p.y, 2.0));
    if (min_scan_range_ < r && r < max_scan_range_)
    {
      scan.push_back(p);
    }
  }

  pcl::PointCloud<pcl::PointXYZI>::Ptr scan_ptr(new pcl::PointCloud<pcl::PointXYZI>(scan));

  // Add initial point cloud to velodyne_map
  // 如果是第一帧点云
  if (initial_scan_loaded == 0)
  {
    pcl::transformPointCloud(*scan_ptr, *transformed_scan_ptr, tf_btol_);  // 第一帧转换到base下
    map_ += *transformed_scan_ptr;
    initial_scan_loaded = 1;
  }

  // 扫描到的点云下采样
  voxel_grid_filter_.setInputCloud(scan_ptr);
  voxel_grid_filter_.filter(*filtered_scan_ptr);
  icp.setInputSource(filtered_scan_ptr);

  // 如果是最开始的第一帧点云的地图,直接设置配准的目标为这个地图
  pcl::PointCloud<pcl::PointXYZI>::Ptr map_ptr(new pcl::PointCloud<pcl::PointXYZI>(map_));
  if (is_first_map_ == true){
    icp.setInputTarget(map_ptr);
    is_first_map_ = false;
  }

  // 初始值为(0,0,0,0,0,0)
  Eigen::Translation3f init_translation(current_pose_.x, current_pose_.y, current_pose_.z);
  Eigen::AngleAxisf init_rotation_x(current_pose_.roll, Eigen::Vector3f::UnitX());
  Eigen::AngleAxisf init_rotation_y(current_pose_.pitch, Eigen::Vector3f::UnitY());
  Eigen::AngleAxisf init_rotation_z(current_pose_.yaw, Eigen::Vector3f::UnitZ());

  // 设置估计的pose 用来配准
  Eigen::Matrix4f init_guess =
      (init_translation * init_rotation_z * init_rotation_y * init_rotation_x).matrix() * tf_btol_; // 默认是0
  
  // 配准后的输出点云
  pcl::PointCloud<pcl::PointXYZI>::Ptr output_cloud(new pcl::PointCloud<pcl::PointXYZI>);
  icp.align(*output_cloud, init_guess);
  t_localizer = icp.getFinalTransformation(); 

  t_base_link = t_localizer * tf_ltob_;       // 变换到第一帧的坐标系下

  pcl::transformPointCloud(*scan_ptr, *transformed_scan_ptr, t_localizer);

  tf::Matrix3x3 mat_b;
  mat_b.setValue(static_cast<double>(t_base_link(0, 0)), static_cast<double>(t_base_link(0, 1)),
                 static_cast<double>(t_base_link(0, 2)), static_cast<double>(t_base_link(1, 0)),
                 static_cast<double>(t_base_link(1, 1)), static_cast<double>(t_base_link(1, 2)),
                 static_cast<double>(t_base_link(2, 0)), static_cast<double>(t_base_link(2, 1)),
                 static_cast<double>(t_base_link(2, 2)));

  // Update current_pose_.
  // 更新当前pose
  current_pose_.x = t_base_link(0, 3);current_pose_.y = t_base_link(1, 3);current_pose_.z = t_base_link(2, 3);
  mat_b.getRPY(current_pose_.roll, current_pose_.pitch, current_pose_.yaw, 1);//mat2rpy

  transform.setOrigin(tf::Vector3(current_pose_.x, current_pose_.y, current_pose_.z));
  q.setRPY(current_pose_.roll, current_pose_.pitch, current_pose_.yaw); //q from rpy
  transform.setRotation(q);//trans from q

  br_.sendTransform(tf::StampedTransform(transform, input->header.stamp, "map", "base_link"));

  // 如果当前帧和上一帧的平移超过了min_add_scan_shift_,那么更新地图,并更新previous帧,发布地图
  // .否则,previous帧不变
  double shift = sqrt(pow(current_pose_.x - previous_pose_.x, 2.0) + pow(current_pose_.y - previous_pose_.y, 2.0));
  if (shift >= min_add_scan_shift_)
  {
    map_ += *transformed_scan_ptr;
    previous_pose_.x = current_pose_.x;previous_pose_.y = current_pose_.y;previous_pose_.z = current_pose_.z;
    previous_pose_.roll = current_pose_.roll;previous_pose_.pitch = current_pose_.pitch;previous_pose_.yaw = current_pose_.yaw;
    icp.setInputTarget(map_ptr);

    sensor_msgs::PointCloud2::Ptr map_msg_ptr(new sensor_msgs::PointCloud2);
    pcl::toROSMsg(*map_ptr, *map_msg_ptr);
    icp_map_pub_.publish(*map_msg_ptr);
  }

  //sensor_msgs::PointCloud2::Ptr map_msg_ptr(new sensor_msgs::PointCloud2);
  //pcl::toROSMsg(*map_ptr, *map_msg_ptr);
  //ndt_map_pub_.publish(*map_msg_ptr);// it makes rviz very slow.

  // 更新当前帧的pose
  current_pose_msg_.header.frame_id = "map";
  current_pose_msg_.header.stamp = input->header.stamp;
  current_pose_msg_.pose.position.x = current_pose_.x;current_pose_msg_.pose.position.y = current_pose_.y;current_pose_msg_.pose.position.z = current_pose_.z;
  current_pose_msg_.pose.orientation.x = q.x();current_pose_msg_.pose.orientation.y = q.y();current_pose_msg_.pose.orientation.z = q.z();current_pose_msg_.pose.orientation.w = q.w();

  current_pose_pub_.publish(current_pose_msg_);

  std::cout << "-----------------------------------------------------------------" << std::endl;
  std::cout << "Sequence number: " << input->header.seq << std::endl;
  std::cout << "Number of scan points: " << scan_ptr->size() << " points." << std::endl;
  std::cout << "Number of filtered scan points: " << filtered_scan_ptr->size() << " points." << std::endl;
  std::cout << "transformed_scan_ptr: " << transformed_scan_ptr->points.size() << " points." << std::endl;
  std::cout << "map: " << map_.points.size() << " points." << std::endl; // 一定是隔一段时间增加一下
  std::cout << "ICP has converged: " << icp.hasConverged() << std::endl;
  std::cout << "Fitness score: " << icp.getFitnessScore() << std::endl;
  //std::cout << "Number of iteration: " << icp.getFinalNumIteration() << std::endl;
  std::cout << "(x,y,z,roll,pitch,yaw):" << std::endl;          // 当前帧的pose
  std::cout << "(" << current_pose_.x << ", " << current_pose_.y << ", " << current_pose_.z << ", " << current_pose_.roll
            << ", " << current_pose_.pitch << ", " << current_pose_.yaw << ")" << std::endl;
  std::cout << "Transformation Matrix:" << std::endl;
  std::cout << t_localizer << std::endl;                        // 当前帧配准的变换矩阵
  std::cout << "shift: " << shift << std::endl;               // 当前帧与之前的关键帧的距离

  // 保存pcd文件
  // pcl::io::savePCDFile ("/home/s/Dataset/pcd/ndt_map.pcd", *map_ptr);


  std::cout << "-----------------------------------------------------------------" << std::endl;
}


