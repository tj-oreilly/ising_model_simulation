#include <vector>

/// @brief A circular queue class.
/// 
/// Wraps around and overwrites oldest data when the queue is full.
/// @tparam TYPE Storage type.
template <typename TYPE>
class cqueue
{
public:
	cqueue()
	{
	}

	cqueue(std::size_t initialSize)
	{
		_queue = std::vector<TYPE>(initialSize);
	}

	cqueue(std::size_t initialSize, const TYPE& defaultVal)
	{
		_queue = std::vector<TYPE>(initialSize, defaultVal);
	}

	void resize(std::size_t size)
	{
		_queue.resize(size);
	}

	bool empty() const
	{
		return _front == _back;
	}

	void push_back(const TYPE& item)
	{
		//Add value
		_queue[_back] = item;

		//Increase back pointer
		if (_back == capacity() - 1)
			_back = 0;
		else
			++_back;

		//If overwriting data
		if (_back == _front)
		{
			if (_front == capacity() - 1)
				_front = 0;
			else
				++_front;
		}
	}

	/// @brief Retrieves the item at the back of the queue
	/// @param offset An offset from the back position
	/// @return The item.
	TYPE back(std::size_t offset = 0) const
	{
		std::size_t index;

		//Out of range
		if (_back == _front || (_back > _front && (_back - _front) < offset) || (_back < _front && (capacity() + _back - _front) < offset))
		{
			throw std::out_of_range("The offset value was out of range");
			return _queue[0];
		}
		else 
		{
			if (_back <= offset)
				index = capacity() - 1 - offset;
			else
				index = _back - 1 - offset;
		}
		
		return _queue[index];
	}

	void pop_back()
	{
		if (empty())
			return;

		if (_back == 0)
			_back = capacity() - 1;
		else
			--_back;
	}

	std::size_t size() const
	{
		if (_back >= _front)
			return _back - _front;
		else
			return capacity() + _back - _front;
	}

private:
	std::size_t capacity() const
	{
		return _queue.size();
	}

	std::vector<TYPE> _queue;
	std::size_t _front = 0;
	std::size_t _back = 0;
};