#include <memory>

/// @brief A circular queue class.
/// 
/// Wraps around and overwrites oldest data when the queue is full.
/// @tparam _Type Storage type.
template <typename _Type>
class cqueue
{
public:
	//Finish writing
	cqueue()
	{
	}

	cqueue(std::size_t initialSize)
	{
		_queue = std::vector<_Type>(initialSize);
	}

	cqueue(std::size_t initialSize, const _Type& defaultVal)
	{
		_queue = std::vector<_Type>(initialSize, defaultVal);
	}

	void resize(std::size_t size)
	{
		_queue.resize(size);
	}

	bool empty() const
	{
		return _front == _back;
	}

	void push_back(const _Type& item)
	{
		//Add value
		_queue[_back] = item;

		//Increase back pointer
		if (_back == size() - 1)
			_back = 0;
		else
			++_back;

		//If overwriting data
		if (_back == _front)
		{
			if (_front == size() - 1)
				_front = 0;
			else
				++_front;
		}
	}

	_Type back() const
	{
		std::size_t index;
		if (_back == 0)
			index = size() - 1;
		else
			index = _back - 1;

		return _queue[index];
	}

	void pop_back()
	{
		if (empty())
			return;

		if (_back == 0)
			_back = size() - 1;
		else
			--_back;
	}

private:
	std::size_t size() const
	{
		return _queue.size();
	}

	std::vector<_Type> _queue;
	std::size_t _front = 0;
	std::size_t _back = 0;
};