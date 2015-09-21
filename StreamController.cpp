#include "StreamController.h"
#define MAX_BUF_SIZE 1800000  //ms   Tsʱ��ļ��㷽�� s * 1000 * 90     *1000��ʾ���� *90��ʾtimescale


std::shared_ptr<StreamContrller> StreamControllerFactory::CreateWantController(STREAMTYPE type)
{
	std::shared_ptr<StreamContrller> controller;
	switch (type)
	{
	case HLS:
	{
		controller = std::make_shared<HLSStrreamController>();
	}break;
	default:
		break;
	}
	return controller;
}

void HLSStrreamController::init(unsigned char* url)
{
	StreamContrller::init(url);
}



void StreamContrller::InitAVDevice()
{

}


HLSStrreamController::HLSStrreamController()
{
	
}

HLSStrreamController::~HLSStrreamController()
{

}

void HLSStrreamController::VideoThreadFunc(playlist video,HLSStrreamController *controller)
{
	std::list<chunk>::iterator itr = video.chunklist.begin();
	while (itr != video.chunklist.end())
	{
		std::shared_ptr<std::vector<unsigned char>> chunkdata;
		HttpDownloader dm;
		dm.init();
		char buf[100] = { 0 };
		sprintf(buf, "%d-%d", itr->byterange_low, itr->byterange_up);
		dm.setDownloadRange(buf);
		dm.startDownload((unsigned char*)itr->url.c_str(), chunkdata);
		std::cout << "download chunk sucess" << std::endl;

		controller->m_tsParser.read(std::string(), chunkdata, false);
		controller->m_tsParser.Parse();
		c_int64 chunkduration  = controller->m_tsParser.GetTsFileDuration();
		/*if (chunkduration < controller->m_videobuffer.GetRemainBufferInSecond())
		{
			while (1)
			{
				PACKET videopacket;
				if (!controller->m_tsParser.GetPacket(TYPE_VIDEO, videopacket))
				{
					std::cout << "packet get end" << std::endl;
					break;
				}
				controller->m_videobuffer.PutIn(videopacket);
			}
		}
		else
		{
			
		}*/
	}
}

void HLSStrreamController::AudioThreadFunc(playlist audio, HLSStrreamController *controller)
{
	std::list<chunk>::iterator itr = audio.chunklist.begin();
	while (itr != audio.chunklist.end())
	{
		std::shared_ptr<std::vector<unsigned char>> chunkdata;
		HttpDownloader dm;
		dm.init();
		char buf[100] = { 0 };
		sprintf(buf, "%d-%d", itr->byterange_low, itr->byterange_up);
		dm.setDownloadRange(buf);
		dm.startDownload((unsigned char*)itr->url.c_str(), chunkdata);
		std::cout << "download chunk sucess" << std::endl;

		controller->m_tsParser.read(std::string(), chunkdata, false);
		controller->m_tsParser.Parse();
		controller->m_audiobufduration += controller->m_tsParser.GetTsFileDuration();
		std::unique_lock<std::mutex> lck(controller->m_audiomutex);
		if (controller->m_audiobufduration > MAX_BUF_SIZE)
		{
			controller->m_cvV.wait(lck);
		}
		while (1)
		{
			PACKET audiopacket;
			if (!controller->m_tsParser.GetPacket(TYPE_AUDIO, audiopacket))
			{
				std::cout << "packet get end" << std::endl;
				break;
			}
			controller->m_audiopacketbuf.push_back(audiopacket);
		}
	}
}

void HLSStrreamController::SubThreadFunc(playlist sub, HLSStrreamController *controller)
{

}

void HLSStrreamController::GetPacketFunc(TRACKTYPE type, HLSStrreamController *controller)
{
	std::list<PACKET> packetbuf;
	switch (type)
	{
	case TYPE_VIDEO:
	{
		packetbuf = controller->m_videopacketbuf;
	}break;
	case TYPE_AUDIO:
	{
		packetbuf = controller->m_audiopacketbuf;
	}break;
	default:break;
	}
	while (1)
	{
		if (!packetbuf.empty())
		{
			std::unique_lock<std::mutex> lck(controller->m_videomutex);
			PACKET packet = packetbuf.front();
			packetbuf.pop_front();
		}
		else
		{
			continue;
		}
	}
	
}

void HLSStrreamController::start()
{
	m_hlsParser.Parser(m_manifestdownloaddata, m_mainfesturl);
	//�������߳�ȥ��audio,video,subititle ��render��decode
	playlist video;
	playlist audio;
	playlist sub;
	m_hlsParser.getSelectTrackPlaylist(video, audio, sub);
	if (!video.chunklist.empty())
	{
		m_hasvideo = true;
		m_readvideothread = std::thread(VideoThreadFunc, video, this);
	}
	else
	{
		m_hasvideo = false;
	}
	if (!audio.chunklist.empty())
	{
		m_hasaudio = true;
		m_readaudiothread = std::thread(AudioThreadFunc, audio, this);
	}
	else
	{
		m_hasaudio = false;
	}
	if (!sub.chunklist.empty())
	{
		m_hassub = true;
		m_readsubthread = std::thread(SubThreadFunc, sub, this);
	}
	else
	{
		m_hassub = false;
	}


	if (m_hasvideo)
	{
		m_getvideothread = std::thread(GetPacketFunc, TYPE_VIDEO, this);
	}
	while (1)
	{

	}

	
}

void HLSStrreamController::getTrackPlayList(playlist& vplist, playlist& aplist, playlist& splist)
{
	m_hlsParser.getSelectTrackPlaylist(vplist, aplist, splist);
}
