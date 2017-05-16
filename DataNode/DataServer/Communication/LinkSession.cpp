#include "LinkSession.h"
#include "../NodeServer.h"


LinkIDSet::LinkIDSet()
{}

LinkIDSet& LinkIDSet::GetSetObject()
{
	static	LinkIDSet	obj;
	return obj;
}

int LinkIDSet::NewLinkID( unsigned int nNewLinkID )
{
	CriticalLock	guard( m_oLock );

	///< IDδ���ӣ���������
	if( m_setLinkID.find( nNewLinkID ) == m_setLinkID.end() )
	{
		m_setLinkID.insert( nNewLinkID );
		return 1;
	}

	return 0;
}

void LinkIDSet::RemoveLinkID( unsigned int nRemoveLinkID )
{
	CriticalLock	guard( m_oLock );

	///< �������ID�������Ƴ�
	if( m_setLinkID.find( nRemoveLinkID ) != m_setLinkID.end() )
	{
		m_setLinkID.erase( nRemoveLinkID );
	}
}

unsigned int LinkIDSet::FetchLinkIDList( unsigned int * lpLinkNoArray, unsigned int uiArraySize )
{
	unsigned int	nLinkNum = 0;				///< ��Ч��·����
	static	int		s_nLastLinkNoNum = 0;		///< ��һ�ε���·����
	CriticalLock	guard( m_oLock );			///< ��

	if( m_setLinkID.size() != s_nLastLinkNoNum )
	{
		DataNodeService::GetSerivceObj().WriteInfo( "LinkIDSet::FetchLinkIDList() : TCP connection number of QServer fluctuated! new no. = %d, old no. = %d", m_setLinkID.size(), s_nLastLinkNoNum );
		s_nLastLinkNoNum = m_setLinkID.size();
	}

	for( std::set<unsigned int>::iterator it = m_setLinkID.begin(); it != m_setLinkID.end() && nLinkNum < uiArraySize; it++ )
	{
		lpLinkNoArray[nLinkNum++] = *it;
	}

	return nLinkNum;
}


LinkSessions::LinkSessions()
 : m_pDatabase( NULL )
{
}

int LinkSessions::Instance()
{
	DataNodeService::GetSerivceObj().WriteInfo( "LinkSessions::Instance() : initializing ......" );

	int		nErrCode = m_oQuotationBuffer.Initialize();

	if( 0 == nErrCode )
	{
		DataNodeService::GetSerivceObj().WriteInfo( "LinkSessions::Instance() : initialized ......" );
	}
	else
	{
		DataNodeService::GetSerivceObj().WriteError( "LinkSessions::Instance() : failed 2 initialize ..." );
		return nErrCode;
	}

	if( 0 != (nErrCode = ImageRebuilder::GetObj().Initialize()) )	///< ����10M�Ŀ������ݻ���(���ڶ��³�ʼ��)
	{
		DataNodeService::GetSerivceObj().WriteError( "LinkSessions::Instance() : failed 2 initialize Image buffer ..." );
		return -100;
	}

	return 0;
}

int LinkSessions::SendData( unsigned int uiLinkNo, unsigned short usMessageNo, unsigned short usFunctionID, const char* lpInBuf, unsigned int uiInSize, unsigned __int64	nSerialNo )
{
	return DataNodeService::GetSerivceObj().SendData( uiLinkNo, usMessageNo, usFunctionID, lpInBuf, uiInSize );
}

int LinkSessions::SendError( unsigned int uiLinkNo, unsigned short usMessageNo, unsigned short usFunctionID, const char* lpErrorInfo )
{
	return DataNodeService::GetSerivceObj().SendError( uiLinkNo, usMessageNo, usFunctionID, lpErrorInfo );
}

void LinkSessions::PushData( unsigned short usMessageNo, unsigned short usFunctionID, const char* lpInBuf, unsigned int uiInSize, bool bPushFlag, unsigned __int64	nSerialNo )
{
	m_oQuotationBuffer.PutMessage( usMessageNo, lpInBuf, uiInSize );
}

int LinkSessions::CloseLink( unsigned int uiLinkNo )
{
	LinkIDSet::GetSetObject().RemoveLinkID( uiLinkNo );

	return 0;
}

void LinkSessions::OnReportStatus( char* szStatusInfo, unsigned int uiSize )
{
//	cs_format(szStatusInfo,uiSize, _T( ":[API/SPI ����],(2)API/SPI ������Ŀ=%d,(2)API/SPI ��֤��Ŀ=%d,[API ���ü���],�µ�Ƶ��=%.2f,����Ƶ��=%.2f,��ѯƵ��=%.2f"), spi_total, spi_auth, freq_insertOrder, freq_cancelOrder, freq_queryOrder);
}

bool LinkSessions::OnCommand( const char* szSrvUnitName, const char* szCommand, char* szResult, unsigned int uiSize )
{

	return false;
}

bool LinkSessions::OnNewLink( unsigned int uiLinkNo, unsigned int uiIpAddr, unsigned int uiPort )
{
	return ImageRebuilder::GetObj().AddNewReqSession( uiLinkNo );
}

void LinkSessions::OnCloseLink( unsigned int uiLinkNo, int iCloseType )
{
	LinkIDSet::GetSetObject().RemoveLinkID( uiLinkNo );
}

bool LinkSessions::OnRecvData( unsigned int uiLinkNo, unsigned short usMessageNo, unsigned short usFunctionID, bool bErrorFlag, const char* lpData, unsigned int uiSize, unsigned int& uiAddtionData )
{

	return false;
}




