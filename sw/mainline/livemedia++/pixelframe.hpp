#pragma once

/*
 	 video frame
 */

class pixelframe :
		public avframe_class_type
		<pixel>
{
private:
	virtual void field_attr_value(pixel &t)
	{
		t.set(std::tuple<int, int, enum AVPixelFormat>(
				raw()->width,
				raw()->height,
				(enum AVPixelFormat)raw()->format));
	}
	void throw_if_not_video()
	{DECLARE_THROW(type() != AVMEDIA_TYPE_VIDEO,
			"pixel frame type no match");}
public:
	pixelframe() = delete;
	pixelframe(pixel &rhs) :
		avframe_class_type()
	{
		if(rhs.can_take())
		{

			raw()->width = rhs.width();
			raw()->height = rhs.height();
			raw()->format = rhs.format();
			av_frame_get_buffer(raw(), 0);

			av_image_fill_arrays(raw()->data,
					raw()->linesize,
					(uint8_t *)rhs.read(),
					rhs.format(),
					rhs.width(),
					rhs.height(),
					1);
		}
	}

	/*
		setup self
	*/
	pixelframe (unsigned width, 
		unsigned height,
		enum AVPixelFormat fmt,
		void *p) : 
		avframe_class_type()
	{
	DECLARE_THROW(width <= 0, "pixelframe invalid width");
	DECLARE_THROW(height <= 0, "pixelframe invalid height");
	DECLARE_THROW(fmt < 0, "pixelframe invalid format");
	DECLARE_THROW(!p, "pixelframe invalid data pointer");
	
	
		raw()->width = width;
		raw()->height = height;
		raw()->format = fmt;
		av_frame_get_buffer(raw(), 0);
		
		av_image_fill_arrays(raw()->data,
				raw()->linesize,
				(uint8_t *)p,
				fmt,
				width,
				height,
				1);
	}

	pixelframe(const  AVFrame &frm) :
		avframe_class_type(frm)
	{
		throw_if_not_video();
	}
	pixelframe(const pixelframe &_class) :
		avframe_class_type(dynamic_cast<const avframe_class_type &>(_class))
	{
		throw_if_not_video();
	}
	virtual ~pixelframe()
	{

	}

	pixelframe &operator = (const  AVFrame &frm)
	{
		avframe_class_type::operator =(frm);
		throw_if_not_video();
		return *this;
	}
	pixelframe &operator = (const pixelframe &_class)
	{
		avframe_class_type::operator =
				(dynamic_cast<const avframe_class_type &>(_class));

		return *this;
	}
	virtual int len()
	{
		enum AVPixelFormat fmt= (enum AVPixelFormat)raw()->format;
		int width = raw()->width;
		int height = raw()->height;
		/*
			 fill in the AVPicture fields, always assume a linesize alignment of 1.
		 */
		int align = 1;
		return av_image_get_buffer_size(fmt,
				width,
				height,
				align);
	}
	virtual void data_copy(uint8_t *ptr, int length)
	{
		DECLARE_THROW(!ptr || length < len(), "pixelframe invalid parameter");

		av_image_copy_to_buffer(ptr,
				length,
							(uint8_t **)raw()->data,
							raw()->linesize,
							(enum AVPixelFormat)raw()->format,
							raw()->width,
							raw()->height,
							1) ;
	}
	virtual uint8_t *data_alloc(const base_allocator &allocator)
	{

		uint8_t *ptr = nullptr;
		int length = len();

		if(length > 0)
		{
			ptr= (uint8_t *)allocator(length);
			av_image_copy_to_buffer(ptr,
					length,
								(uint8_t **)raw()->data,
								raw()->linesize,
								(enum AVPixelFormat)raw()->format,
								raw()->width,
								raw()->height,
								1) ;
		}
		return ptr;
	}

};


/*
 	 	 using presentation time stamp
 */
struct pixelframe_pts : public pixelframe
{
	double _pts;
	pixelframe_pts(const pixelframe &f) : pixelframe(f), _pts(0.0){}
	void operator()(const AVRational &rational)
	{
			if(raw()->best_effort_timestamp != AV_NOPTS_VALUE)
			{
				_pts = av_q2d(rational) * _pts;
			}
	}

};


