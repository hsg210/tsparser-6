#include "AACParser.h"

AACParser::AACParser()
{

}


AACParser::~AACParser()
{

}

int AACParser::Parse()
{
	std::shared_ptr<std::vector<unsigned char>> data;
	data = m_fileoperator->getAllBytes();
	if (m_id3parser.checkID3(data))
	{
		m_id3parser.parse(data);
	}
	SpiltADTSTrack(data);
	return PARSER_FAIL;
}

void AACParser::SetChunkDuration(double duration)
{
	m_chunkduration = duration;
}

bool AACParser::SpiltADTSTrack(std::shared_ptr<std::vector<unsigned char>> data)
{
	std::shared_ptr<std::list<PACKET>> tmpaudiopacketbuf = std::make_shared<std::list<PACKET>>();
	while (!data->empty())
	{
		ADTSParser adtsparse;
		if (adtsparse.Parser(data) == false)
		{
			std::cout << "atds parser take error" << std::endl;
			break;
		}
		int num = adtsparse.GetAACWholeDataSize();
		//packetendnum += num;
		PACKET audiopacket;
		std::shared_ptr<std::vector<unsigned char>> packetdata = std::make_shared<std::vector<unsigned char>>();
		if (num > data->size())
		{
			num = data->size();
		}
		packetdata->assign(data->begin(), data->begin() + num);  //assign ��Χ���ڶ�������֮ǰһ������Ҫ���һ��1
		audiopacket.data = packetdata;
		audiopacket.size = packetdata->size();
		//audiopacket.pts = pesdata->pts;
		tmpaudiopacketbuf->push_back(audiopacket);
		std::vector<unsigned char>::iterator tmpite = data->begin() + num;
		if (num != data->size())
		{
			if (*tmpite != 0xff && (*(tmpite++) & 0xf0) != 0xf0)
			{
				std::cout << "fuck" << std::endl;
			}
		}
		data->erase(data->begin(), data->begin() + num);
		//��ʼ�Էֽ����һ֡AAC��������װ
	}
	CalcADTSFramePts(tmpaudiopacketbuf);
	m_audioPacketBuf->insert(m_audioPacketBuf->end(), tmpaudiopacketbuf->begin(), tmpaudiopacketbuf->end());
	return true;
}

void AACParser::CalcADTSFramePts(std::shared_ptr<std::list<PACKET>> audiopacketbuf)
{
	int pts = m_chunkduration * 1000 * 90;
	pts = pts / audiopacketbuf->size();
	double startpts = m_id3parser.GetId3().pts;
	std::list<PACKET>::iterator itr = audiopacketbuf->begin();
	for (; itr != audiopacketbuf->end(); itr++)
	{
		if (itr == audiopacketbuf->begin())
		{
			itr->pts = startpts;
		}
		else
		{
			startpts += pts;
			itr->pts += startpts;
		}
	}
}