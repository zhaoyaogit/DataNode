#pragma warning(disable:4996)
#include <time.h>
#include "LinkSession.h"
#include "../DataEcho.h"
#include "../NodeServer.h"
#include "../../../../DataCluster/DataCluster/Protocal/DataCluster_Protocal.h"


LinkNoRegister::LinkNoRegister()
 : m_nLinkCount( 0 )
{}

LinkNoRegister& LinkNoRegister::GetRegister()
{
	static LinkNoRegister		obj;

	return obj;
}

int LinkNoRegister::NewPushLinkID( unsigned int nNewLinkID )
{
	CriticalLock	guard( m_oLock );

	///< ID未添加，可以添加
	if( m_setPushLinkID.find( nNewLinkID ) == m_setPushLinkID.end() )
	{
		m_setPushLinkID.insert( nNewLinkID );
		m_nLinkCount = m_setPushLinkID.size();

		int		n = 0;
		for( std::set<unsigned int>::iterator it = m_setPushLinkID.begin(); it != m_setPushLinkID.end() && n < MAX_LINKID_NUM; it++ )
		{
			m_vctLinkNo[n++] = *it;
		}

		return 1;
	}

	return 0;
}

void LinkNoRegister::RemovePushLinkID( unsigned int nRemoveLinkID )
{
	unsigned int	nLinkNum = 0;				///< 有效链路数量
	CriticalLock	guard( m_oLock );

	///< 存在这个ID，可以移除
	if( m_setPushLinkID.find( nRemoveLinkID ) != m_setPushLinkID.end() )
	{
		m_setPushLinkID.erase( nRemoveLinkID );
		m_nLinkCount = m_setPushLinkID.size();

		int		n = 0;
		for( std::set<unsigned int>::iterator it = m_setPushLinkID.begin(); it != m_setPushLinkID.end() && n < MAX_LINKID_NUM; it++ )
		{
			m_vctLinkNo[n++] = *it;
		}
	}
}

unsigned int LinkNoRegister::FetchLinkNoTable( unsigned int* pIDTable, unsigned int nBuffSize )
{
	CriticalLock	guard( m_oLock );

	::memcpy( pIDTable, m_vctLinkNo+0, m_nLinkCount*sizeof(unsigned int) );

	return m_nLinkCount;
}

int LinkNoRegister::GetPushLinkCount()
{
	CriticalLock	guard( m_oLock );

	return m_nLinkCount;
}

int LinkNoRegister::NewReqLinkID( unsigned int nReqLinkID )
{
	CriticalLock	guard( m_oLock );			///< 锁

	///< ID未添加，可以添加
	if( m_setNewReqLinkID.find( nReqLinkID ) == m_setNewReqLinkID.end() )
	{
		m_setNewReqLinkID.insert( nReqLinkID );
		return 1;
	}

	return 0;
}

int LinkNoRegister::GetReqLinkCount()
{
	CriticalLock	guard( m_oLock );			///< 锁

	return m_setNewReqLinkID.size();
}

int LinkNoRegister::PopReqLinkID()
{
	int									nReqLinkID = 0;
	CriticalLock						guard( m_oLock );			///< 锁

	if( m_setNewReqLinkID.empty() )
	{
		return -1;
	}

	std::set<unsigned int>::iterator	it = m_setNewReqLinkID.begin();

	nReqLinkID = *it;
	m_setNewReqLinkID.erase( it );		///< 将新会话的id删除

	return nReqLinkID;
}

bool LinkNoRegister::InReqLinkIDSet( unsigned int nLinkID )
{
	CriticalLock						guard( m_oLock );			///< 锁

	if( m_setNewReqLinkID.find( nLinkID ) != m_setNewReqLinkID.end() )
	{
		return true;
	}

	return false;
}


SessionCollection::SessionCollection( PowerfullDatabase& refDbIO )
 : m_refDatabase( refDbIO )
{
}

SessionCollection::~SessionCollection()
{
}

int SessionCollection::Instance()
{
	DataNodeService::GetSerivceObj().WriteInfo( "SessionCollection::Instance() : initializing ......" );

	Release();

	int	nErrCode = m_oQuotationBuffer.Initialize();
	if( 0 == nErrCode )
	{
		DataNodeService::GetSerivceObj().WriteInfo( "SessionCollection::Instance() : initialized ......" );
	}
	else
	{
		DataNodeService::GetSerivceObj().WriteError( "SessionCollection::Instance() : failed 2 initialize ..., errorcode=%d", nErrCode );
		return nErrCode;
	}

	return 0;
}

void SessionCollection::Release()
{
	m_oQuotationBuffer.Release();
}

void SessionCollection::PushQuotation( unsigned short usMessageNo, unsigned short usFunctionID, const char* lpInBuf, unsigned int uiInSize, bool bPushFlag, unsigned __int64 nSerialNo )
{
	m_oQuotationBuffer.PutMessage( usMessageNo, lpInBuf, uiInSize, nSerialNo );
}

void SessionCollection::OnReportStatus( char* szStatusInfo, unsigned int uiSize )
{
	char			pszStatusDesc[1024*2] = { 0 };
	unsigned int	nDescLen = sizeof(pszStatusDesc);
	unsigned int	nModuleVersion = Configuration::GetConfigObj().GetStartInParam().uiVersion;
	float			dFreePer = m_oQuotationBuffer.GetFreePercent();

	::sprintf( szStatusInfo
		, ":working = %s,[NodeServer],版本 = V%.2f B%03d,测试行情模式 = %s,发送心跳包数 = %u,推送链路数 = %d(路),\
		  初始化链路数 = %u(路),数据表数量 = %u(张), 缓存空闲比例 = %.2f(％)\
		  ,[QuotationPlugin],%s"
		, DataNodeService::GetSerivceObj().OnInquireStatus( pszStatusDesc, nDescLen )==true?"true":"false"
		, (float)(nModuleVersion>>16)/100.f, nModuleVersion&0xFF
		, Configuration::GetConfigObj().GetTestFlag()==true?"是":"否"
		, DataNodeService::GetSerivceObj().OnInquireHeartBeatCount()
		, LinkNoRegister::GetRegister().GetPushLinkCount(), LinkNoRegister::GetRegister().GetReqLinkCount()
		, m_refDatabase.GetTableCount(), dFreePer
		, pszStatusDesc );
}

bool SessionCollection::OnNewLink( unsigned int uiLinkNo, unsigned int uiIpAddr, unsigned int uiPort )
{
	char			pszStatusDesc[1024*2] = { 0 };
	unsigned int	nDescLen = sizeof(pszStatusDesc);
	bool			bStatusIsOkey = DataNodeService::GetSerivceObj().OnInquireStatus( pszStatusDesc, nDescLen );

	if( false == bStatusIsOkey )
	{
		DataNodeService::GetSerivceObj().WriteWarning( "SessionCollection::OnNewLink() : TCP connection request has been denied! (linkno. = %u) Service isn\'t available... ", uiLinkNo );
	}

	return bStatusIsOkey;
}

void SessionCollection::OnCloseLink( unsigned int uiLinkNo, int iCloseType )
{
	DataNodeService::GetSerivceObj().WriteWarning( "SessionCollection::OnCloseLink() : TCP connection closed! (linkno. = %u) errorcode=%d", uiLinkNo, iCloseType );

	LinkNoRegister::GetRegister().RemovePushLinkID( uiLinkNo );
}

bool SessionCollection::OnRecvData( unsigned int uiLinkNo, unsigned short usMessageNo, unsigned short usFunctionID, bool bErrorFlag, const char* lpData, unsigned int uiSize, unsigned int& uiAddtionData )
{
	if( MSG_LOGIN_ID == usMessageNo )
	{
		tagPackageHead*				pHead = (tagPackageHead*)lpData;
		tagCommonLoginData_LF299*	pMsgBody = (tagCommonLoginData_LF299*)( lpData + sizeof(tagPackageHead) );

		::strcpy( pMsgBody->pszActionKey, "success" );

		int	nErrCode = DataNodeService::GetSerivceObj().SendData( uiLinkNo, usMessageNo, usFunctionID, lpData, sizeof(tagCommonLoginData_LF299) + sizeof(tagPackageHead) );
		if( nErrCode < 0 )
		{
			DataNodeService::GetSerivceObj().WriteWarning( "SessionCollection::OnRecvData() : failed 2 reply login request, errorcode=%d", nErrCode );
			return false;
		}

		///< ------------ 将校验通过的请求，加入待初始化列表 ------------------
		if( true == LinkNoRegister::GetRegister().InReqLinkIDSet( uiLinkNo ) )
		{
			DataNodeService::GetSerivceObj().WriteInfo( "SessionCollection::OnRecvData() : [WARNING] duplicate link number & new link will be disconnected..." );
			return false;
		}

		LinkNoRegister::GetRegister().NewReqLinkID( uiLinkNo );
		DataNodeService::GetSerivceObj().WriteInfo( "SessionCollection::OnRecvData() : [NOTICE] link[%u] logged 2 server successfully." );

		return true;
	}

	return false;
}

bool SessionCollection::OnCommand( const char* szSrvUnitName, const char* szCommand, char* szResult, unsigned int uiSize )
{
	int							nArgc = 32;
	char*						pArgv[32] = { 0 };
	unsigned int				nMarketID = DataCollector::GetMarketID();

	///< 拆解出关键字和参数字符
	if( false == SplitString( pArgv, nArgc, szCommand ) )
	{
		::sprintf( szResult, "RealTimeQuote4LinksSpi::OnCommand : [ERR] parse command string failed, [%s]", szCommand );
		return true;
	}

	///< 先判断是否为系统控制命令，如果是就执行，否则继续往下执行，判断是否为回显命令
	if( true == ModuleControl::GetSingleton()( pArgv, nArgc, szResult, uiSize ) )
	{
		return true;
	}

	///< 根据挂载的数据采集器对应的市场ID，使用对应的数据监控
	switch( nMarketID )
	{
	case QUO_MARKET_DCE:
		return DLFuture_Echo::GetSingleton()( pArgv, nArgc, szResult, uiSize );		///< 执行回显命令串
	case QUO_MARKET_DCEOPT:
		return DLOption_Echo::GetSingleton()( pArgv, nArgc, szResult, uiSize );		///< 执行回显命令串
	case QUO_MARKET_SHFE:
		return SHFuture_Echo::GetSingleton()( pArgv, nArgc, szResult, uiSize );		///< 执行回显命令串
	case QUO_MARKET_SHFEOPT:
		return  SHOption_Echo::GetSingleton()( pArgv, nArgc, szResult, uiSize );	///< 执行回显命令串
	case QUO_MARKET_CZCE:
		return ZZFuture_Echo::GetSingleton()( pArgv, nArgc, szResult, uiSize );		///< 执行回显命令串
	case QUO_MARKET_CZCEOPT:
		return ZZOption_Echo::GetSingleton()( pArgv, nArgc, szResult, uiSize );		///< 执行回显命令串
	case QUO_MARKET_CFFEX:
		return CFFFuture_Echo::GetSingleton()( pArgv, nArgc, szResult, uiSize );	///< 执行回显命令串
	case QUO_MARKET_SSE:
		return SHL1_Echo::GetSingleton()( pArgv, nArgc, szResult, uiSize );			///< 执行回显命令串
	case QUO_MARKET_SSEOPT:
		return SHL1Option_Echo::GetSingleton()( pArgv, nArgc, szResult, uiSize );	///< 执行回显命令串
	case QUO_MARKET_SZSE:
		return SZL1_Echo::GetSingleton()( pArgv, nArgc, szResult, uiSize );			///< 执行回显命令串
	default:
		::sprintf( szResult, "不能识别命令[%s]或市场ID[%u]", szCommand, nMarketID );
		break;
	}

	return true;
}












