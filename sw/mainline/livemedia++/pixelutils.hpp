#pragma once
class pixelutils : public pixel
{
public:
	pixelutils() :
		pixel()
	{
	}
	virtual ~pixelutils()
	{

	}
	bool imagefile(char const *file)
	{
		if(!file)
		{
			return false;
		}
		mediacontainer con(file);
		if(!con.find_codec(AVMEDIA_TYPE_VIDEO))
		{
			return false;
		}
		decoder dec(con.find_stream(AVMEDIA_TYPE_VIDEO)->codecpar);

		while(!can_take())
		{
			avpacket_class pkt;
			if(!con.read_packet(AVMEDIA_TYPE_VIDEO, pkt))
			{
				break;
			}
			printf("readpkt \n");
			dec.decoding(pkt, [&](avpacket_class &packet,
					avframe_class & frm, void *ptr)->void{
				pixelframe pixfrm(*frm.raw());
				pixfrm >> *this;
				printf("ok!!!!!\n");
			});
		}

		return can_take();
	}
	bool convert(int width, int height, enum AVPixelFormat fmt)
	{
		if(!can_take())
		{
			return false;
		}
		pixelframe pixfrm(*this);

		avattr attr;
		attr.set(avattr_key::frame_video, avattr_key::frame_video, 0, 0.0);
		attr.set(avattr_key::width, avattr_key::width, width, 0.0);
		attr.set(avattr_key::height, avattr_key::height, height, 0.0);
		attr.set(avattr_key::pixel_format, avattr_key::pixel_format, (int)fmt, 0.0);

		swxcontext_class (pixfrm,
				attr);
		pixfrm >> *this;
		return true;
	}
};
