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

	///< ID未添加，可以添加
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

	///< 存在这个ID，可以移除
	if( m_setLinkID.find( nRemoveLinkID ) != m_setLinkID.end() )
	{
		m_setLinkID.erase( nRemoveLinkID );
	}
}

unsigned int LinkIDSet::FetchLinkIDList( unsigned int * lpLinkNoArray, unsigned int uiArraySize )
{
	unsigned int	nLinkNum = 0;				///< 有效链路数量
	static	int		s_nLastLinkNoNum = 0;		///< 上一次的链路数量
	CriticalLock	guard( m_oLock );			///< 锁

	if( m_setLinkID.size() != s_nLastLinkNoNum )
	{
		SvrFramework::GetFramework().WriteInfo( "LinkIDSet::FetchLinkIDList() : TCP connection number of QServer fluctuated! new no. = %d, old no. = %d", m_setLinkID.size(), s_nLastLinkNoNum );
		s_nLastLinkNoNum = m_setLinkID.size();
	}

	for( std::set<unsigned int>::iterator it = m_setLinkID.begin(); it != m_setLinkID.end() && nLinkNum < uiArraySize; it++ )
	{
		lpLinkNoArray[nLinkNum++] = *it;
	}

	return nLinkNum;
}


#define		MAX_IMAGE_BUFFER_SIZE			(1024*1024*10)


LinkSessionSet::LinkSessionSet()
 : m_pDatabase( NULL )
{
}

LinkSessionSet& LinkSessionSet::GetSessionSet()
{
	static	LinkSessionSet	obj;

	return obj;
}

int LinkSessionSet::Instance()
{
	SvrFramework::GetFramework().WriteInfo( "LinkSessionSet::Instance() : initializing ......" );

	int		nErrCode = m_oQuotationBuffer.Initialize();

	if( 0 == nErrCode )
	{
		SvrFramework::GetFramework().WriteInfo( "LinkSessionSet::Instance() : initialized ......" );
	}
	else
	{
		SvrFramework::GetFramework().WriteError( "LinkSessionSet::Instance() : failed 2 initialize ..." );
		return nErrCode;
	}

	m_pImageDataBuffer = new char[MAX_IMAGE_BUFFER_SIZE];	///< 分配10M的快照数据缓存(用于对下初始化)
	if( NULL == m_pImageDataBuffer )
	{
		SvrFramework::GetFramework().WriteError( "LinkSessionSet::Instance() : failed 2 initialize Image buffer ..." );
		return -10;
	}

	return 0;
}

int LinkSessionSet::SendData( unsigned int uiLinkNo, unsigned short usMessageNo, unsigned short usFunctionID, const char* lpInBuf, unsigned int uiInSize )
{
	return SvrFramework::GetFramework().SendData( uiLinkNo, usMessageNo, usFunctionID, lpInBuf, uiInSize );
}

int LinkSessionSet::SendError( unsigned int uiLinkNo, unsigned short usMessageNo, unsigned short usFunctionID, const char* lpErrorInfo )
{
	return SvrFramework::GetFramework().SendError( uiLinkNo, usMessageNo, usFunctionID, lpErrorInfo );
}

void LinkSessionSet::PushData( unsigned short usMessageNo, unsigned short usFunctionID, const char* lpInBuf, unsigned int uiInSize, bool bPushFlag )
{
	m_oQuotationBuffer.PutMessage( usMessageNo, lpInBuf, uiInSize );
}

int LinkSessionSet::CloseLink( unsigned int uiLinkNo )
{
	LinkIDSet::GetSetObject().RemoveLinkID( uiLinkNo );

	return 0;
}

void LinkSessionSet::OnReportStatus( char* szStatusInfo, unsigned int uiSize )
{
//	cs_format(szStatusInfo,uiSize, _T( ":[API/SPI 总数],(2)API/SPI 创建数目=%d,(2)API/SPI 验证数目=%d,[API 调用计数],下单频率=%.2f,撤单频率=%.2f,查询频率=%.2f"), spi_total, spi_auth, freq_insertOrder, freq_cancelOrder, freq_queryOrder);
}

bool LinkSessionSet::OnCommand( const char* szSrvUnitName, const char* szCommand, char* szResult, unsigned int uiSize )
{

	return false;
}

bool LinkSessionSet::OnNewLink( unsigned int uiLinkNo, unsigned int uiIpAddr, unsigned int uiPort )
{
	if( 1 == LinkIDSet::GetSetObject().NewLinkID( uiLinkNo ) )
	{
		unsigned int		lstTableID[64] = { 0 };
		DatabaseIO&			refDatabaseIO = DataNodeService::GetSerivceObj().GetDatabaseIO();
		unsigned int		nTableCount = refDatabaseIO.GetTablesID( lstTableID, 64 );

		for( int n = 0; n < nTableCount; n++ )
		{
			unsigned int	nTableID = lstTableID[n];
			int				nDataLen = refDatabaseIO.FetchDataBlockByID( nTableID, m_pImageDataBuffer, MAX_IMAGE_BUFFER_SIZE );

			if( nDataLen < 0 )
			{
				SvrFramework::GetFramework().WriteWarning( "LinkSessionSet::OnNewLink() : failed 2 fetch records of table, errorcode=%d", nDataLen );
				return false;
			}

			nDataLen = SendData( uiLinkNo, 0, 0, m_pImageDataBuffer, nDataLen );
			if( nDataLen < 0 )
			{
				SvrFramework::GetFramework().WriteWarning( "LinkSessionSet::OnNewLink() : failed 2 send image data, errorcode=%d", nDataLen );
				return false;
			}
		}

		return true;
	}

	SvrFramework::GetFramework().WriteInfo( "LinkSessionSet::OnNewLink() : [WARNING] duplicate link number & new link will be disconnected..." );

	return false;
}

void LinkSessionSet::OnCloseLink( unsigned int uiLinkNo, int iCloseType )
{
	LinkIDSet::GetSetObject().RemoveLinkID( uiLinkNo );
}

bool LinkSessionSet::OnRecvData( unsigned int uiLinkNo, unsigned short usMessageNo, unsigned short usFunctionID, bool bErrorFlag, const char* lpData, unsigned int uiSize, unsigned int& uiAddtionData )
{

	return false;
}





