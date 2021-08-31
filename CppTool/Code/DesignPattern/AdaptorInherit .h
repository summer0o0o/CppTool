#pragma once
#include <iostream>
using namespace std;

//˫�˶��У���������
class Deque
{
public:
	void push_back(int x)
	{
		cout << "Deque push_back:" << x << endl;
	}
	void push_front(int x)
	{
		cout << "Deque push_front:" << x << endl;
	}
	void pop_back()
	{
		cout << "Deque pop_back" << endl;
	}
	void pop_front()
	{
		cout << "Deque pop_front" << endl;
	}
};

//˳���࣬����Ŀ����
class Sequence
{
public:
	virtual void push(int x) = 0;
	virtual void pop() = 0;
};

//ջ,����ȳ�, ������
class Stack :public Sequence, private Deque
{
public:
	void push(int x) override
	{
		push_front(x);
	}
	void pop() override
	{
		pop_front();
	}
};

//���У��Ƚ��ȳ���������
class Queue :public Sequence, private Deque
{
public:
	void push(int x) override
	{
		push_back(x);
	}
	void pop() override
	{
		pop_front();
	}
};