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


/*
 	 	 audio raw data class
 */
class pcm :
		public raw_media_data
{
private:
	std::tuple <int,/*channel*/
	int,/*samplingrate*/
	int,/*samplesize*/
	enum AVSampleFormat/*sample format*/
	> _val;
public:
	/*
	 	 because reqeust size always > 0
	 */
	pcm() : raw_media_data(AVMEDIA_TYPE_AUDIO),
	_val(std::make_tuple(0, 0, 0, AV_SAMPLE_FMT_NONE)) { }
	pcm(const pcm &rhs) :
		raw_media_data (dynamic_cast<const raw_media_data &>(rhs)),
		_val(rhs._val) { }
	virtual ~pcm(){}
	pcm &operator =
			(const pcm &rhs)
	{
		raw_media_data::operator =
				(dynamic_cast<const raw_media_data &>(rhs));
		_val = rhs._val;
		return *this;
	}
	void set(int channel,
			int samplingrate,
			int samplesize,
			enum AVSampleFormat format)
	{
		_val = std::make_tuple(channel,
				samplingrate,
				samplesize,
				format);
	}
	void set(std::tuple<int,
			int,
			int,
			enum AVSampleFormat> &&val)
	{
		_val = val;
	}

	int channel() const
	{ return std::get<0>(_val); }
	int samplingrate() const
	{ return std::get<1>(_val); }
	int samplesize() const
	{ return std::get<2>(_val); }
	enum AVSampleFormat format() const
	{ return std::get<3>(_val); }
	virtual bool can_take()
	{
		/*
		 	 responsor filled field
		 */
		return std::get<0>(_val) > 0 &&
				std::get<1>(_val) > 0 &&
				std::get<2>(_val) > 0 &&
				std::get<3>(_val) != AV_SAMPLE_FMT_NONE &&
				raw_media_data::can_take();
	}
	pcm *clone()
	{
		return new pcm(*this);
	}
};
/*
 	 video raw data class
 */
class pixel :
		public raw_media_data
{
private:
	std::tuple<int, int, enum AVPixelFormat> _val;
public:
	pixel() : raw_media_data(AVMEDIA_TYPE_VIDEO),
			_val(std::make_tuple(0, 0, AV_PIX_FMT_NONE)){}
	pixel(const pixel &rhs) :
		raw_media_data(dynamic_cast<const raw_media_data &>(rhs)),
		_val(rhs._val){}
	virtual ~pixel(){}
	pixel &operator =
			(const pixel &rhs)
	{
		raw_media_data::operator =
				(dynamic_cast<const raw_media_data &>(rhs));
		_val = rhs._val;
		return *this;
	}
	void set(int width, int
			height,
			enum AVPixelFormat format)
	{
		_val = std::make_tuple(width, height, format);
	}
	void set(std::tuple<int,
			int,
			enum AVPixelFormat> &&val)
	{
		_val = val;
	}

	int width() const
	{ return std::get<0>(_val); }
	int height() const
	{ return std::get<1>(_val); }
	enum AVPixelFormat format() const
	{ return std::get<2>(_val); }
	virtual bool can_take()
	{
		return  std::get<0>(_val) > 0 &&
				std::get<1>(_val) > 0 &&
				std::get<2>(_val) != AV_PIX_FMT_NONE &&
				raw_media_data::can_take();
	}
	pixel *clone()
	{
		return new pixel(*this);
	}
};
