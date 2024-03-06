#include <memory>

/// @brief Circular queue class.
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
		
	}

	bool empty();
	void push_back(const _Type& item);
	_Type back();
	void pop_back();

private:
	std::unique_ptr<_Type[]> _queue;
};