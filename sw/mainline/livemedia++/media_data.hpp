#pragma once

class raw_media_data
{
private:
	const base_allocator _allocator;
	const base_deallocator _deallocator;
	uint8_t *_data_ptr;
	int _data_size;
   double _pts;/*set presentation time if known*/
   enum AVMediaType _type;
public :
	raw_media_data(enum AVMediaType t) :
		_allocator(__base__malloc__),
		_deallocator(__base__free__),
		_data_ptr(nullptr),
                _data_size(0),_pts(0.0), _type(t) { }
	raw_media_data
	(const raw_media_data &rhs) :
		_allocator(__base__malloc__),
		_deallocator(__base__free__),
		_data_ptr(nullptr),
                _data_size(0),_pts(0.0),_type(rhs._type)
	{
		if(rhs._data_ptr &&
				rhs._data_size > 0)
		{
			_data_size = rhs._data_size;
			_data_ptr = (uint8_t *)_allocator(_data_size);
			memcpy(_data_ptr, rhs._data_ptr, _data_size);
                        _pts = rhs._pts;
		}
	}
	raw_media_data(raw_media_data &&rhs) :
		_allocator(__base__malloc__),
		_deallocator(__base__free__),
		_data_ptr(rhs._data_ptr),
      _data_size(rhs._data_size),
		_pts(rhs._pts),
		_type(rhs._type)
	{
		/* move */
		rhs._data_ptr = nullptr;
		rhs._data_size = 0;
	}
	virtual ~raw_media_data()
	{
		if(_data_ptr)
		{
			_deallocator(_data_ptr);
		}
		_data_ptr = nullptr;
		_data_size = 0;
	}
        void setpts(double pts)
        {
           _pts = pts;
        }
        double getpts()
        {
            return _pts;
        }

	const base_allocator &allocator() const
	{
		return _allocator;
	}
	raw_media_data &operator = (raw_media_data &&rhs)
	{
		if(_data_ptr)
		{
			_deallocator(_data_ptr);
		}
		_data_ptr = nullptr;
		_data_size = 0;

		_data_ptr = rhs._data_ptr;
      _data_size = rhs._data_size;
		_pts = rhs._pts;
		_type = rhs._type;
		rhs._data_ptr = nullptr;
		rhs._data_size = 0;
		return *this;
	}

	raw_media_data &operator = (const raw_media_data &rhs)
	{
		if(_data_ptr)
		{
			_deallocator(_data_ptr);
		}
		_data_ptr = nullptr;
		_data_size = 0;

		if(rhs._data_ptr &&
				rhs._data_size > 0)
		{
			_data_size = rhs._data_size;
			_data_ptr = (uint8_t *)_allocator(_data_size);
			memcpy(_data_ptr, rhs._data_ptr, _data_size);
		}
		return *this;
	}
	void replace(uint8_t *ptr, size_t size)
	{
		if(ptr &&
				size > 0)
		{
			if(_data_ptr)
			{
				_deallocator(_data_ptr);
			}
			_data_ptr = ptr;
			_data_size = size;
		}
	}
	void write(uint8_t *ptr, size_t size)
	{
		if(ptr &&
				size > 0)
		{
			if(_data_ptr)
			{
				_deallocator(_data_ptr);
			}

			_data_ptr = (uint8_t *)_allocator(size);
			_data_size = size;
			memcpy(_data_ptr, ptr, _data_size);

		}
	}
	void copyto(uint8_t *ptr, size_t size)
	{
		if(!ptr ||
				size <= 0)
		{
			return;
		}
		int minsize = std::min(_data_size, (int)size);
		memcpy(ptr, _data_ptr,  minsize);
	}
	enum AVMediaType type()
	{
		return _type;
	}
	raw_media_data *clone()
	{
		return new raw_media_data(*this);
	}
	uint8_t const *read() const
	{
		return (uint8_t const * )_data_ptr;
	}
	int size() const
	{
		return _data_size;
	}
	virtual bool can_take()
	{

		return read() != nullptr &&
				size() > 0;
	}
};
