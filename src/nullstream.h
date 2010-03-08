
#ifndef __NULLSTREAM_H__
#define __NULLSTREAM_H__

//This source has been taken from: http://bytes.com/topic/c/answers/127843-null-output-stream

struct nullstream : std::ostream 
{
	struct nullbuf: std::streambuf 
	{
			int overflow(int c) { return traits_type::not_eof(c); }
	} m_sbuf;

	nullstream() : std::ios(&m_sbuf), std::ostream(&m_sbuf) 
	{ }
};

#endif

