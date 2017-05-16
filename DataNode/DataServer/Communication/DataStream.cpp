#include "DataStream.h"
#include "LinkSession.h"
#include "../NodeServer.h"
#pragma warning(disable:4244)


QuotationStream::QuotationStream()
 : m_pSendBuffer( NULL ), m_nMaxSendBufSize( 0 )
{
}

QuotationStream::~QuotationStream()
{
	Release();
}

void QuotationStream::Release()
{
	CriticalLock	guard( m_oLock );

	SimpleTask::StopThread();	///< ֹͣ�߳�
	SimpleTask::Join();			///< �˳��ȴ�
	m_oDataBuffer.Release();	///< �ͷ�������Դ

	if( NULL != m_pSendBuffer )
	{
		delete []m_pSendBuffer;
		m_pSendBuffer = NULL;
	}

	m_nMaxSendBufSize = 0;
}

int QuotationStream::Initialize( unsigned int nNewBuffSize )
{
	Release();

	CriticalLock	guard( m_oLock );

	if( NULL == (m_pSendBuffer = new char[nNewBuffSize]) )
	{
		DataNodeService::GetSerivceObj().WriteError( "QuotationStream::Instance() : failed 2 initialize send data buffer, size = %d", nNewBuffSize );
		return -1;
	}
	m_nMaxSendBufSize = nNewBuffSize;

	if( 0 != m_oDataBuffer.Initialize( nNewBuffSize ) )	///< ���ڴ������һ�黺��
	{
		DataNodeService::GetSerivceObj().WriteError( "QuotationStream::Instance() : failed 2 allocate cache, size = %d", nNewBuffSize );
		return -2;
	}

	return 0;
}

int QuotationStream::Execute()
{
	while( true )
	{
		if( true == m_oDataBuffer.IsEmpty() )	{
			m_oWaitEvent.Wait();
		}

		FlushQuotation2Client();		///< ѭ�����ͻ����е�����
	}

	return 0;
}

int QuotationStream::PutMessage( unsigned short nMsgID, const char *pData, unsigned int nLen )
{
	if( NULL == pData || 0 == nLen )
	{
		return -12345;
	}

	CriticalLock	guard( m_oLock );
	bool			bNeedActivateEvent = m_oDataBuffer.IsEmpty();			///< �Ƿ���Ҫ�����¼�����
	int				nErrorCode = m_oDataBuffer.PutData( pData, nLen );		///< ��������

	if( true == bNeedActivateEvent )
	{
		m_oWaitEvent.Active();
	}

	return nErrorCode;
}

void QuotationStream::FlushQuotation2Client()
{
	LinkIDSet::LINKID_VECTOR	vctLinkID;
	CriticalLock				guard( m_oLock );
	unsigned int				nLinkCount = LinkIDSet::GetSetObject().FetchLinkIDList( vctLinkID+0, 32 );

	if( false == m_oDataBuffer.IsEmpty() && nLinkCount > 0 )
	{
		int	nDataSize = m_oDataBuffer.GetData( m_pSendBuffer, m_nMaxSendBufSize );
		DataNodeService::GetSerivceObj().PushData( vctLinkID+0, nLinkCount, 0, 0, m_pSendBuffer, nDataSize );
	}
}


#define		MAX_IMAGE_BUFFER_SIZE			(1024*1024*10)


ImageRebuilder::ImageRebuilder()
 : m_pImageDataBuffer( NULL )
{
}

ImageRebuilder& ImageRebuilder::GetObj()
{
	static ImageRebuilder		obj;

	return obj;
}

void ImageRebuilder::Release()
{
	if( NULL != m_pImageDataBuffer )
	{
		delete [] m_pImageDataBuffer;
		m_pImageDataBuffer = NULL;
	}
}

int ImageRebuilder::Initialize()
{
	Release();
	m_pImageDataBuffer = new char[MAX_IMAGE_BUFFER_SIZE];	///< ����10M�Ŀ������ݻ���(���ڶ��³�ʼ��)

	if( NULL == m_pImageDataBuffer )
	{
		DataNodeService::GetSerivceObj().WriteError( "ImageRebuilder::Initialize() : failed 2 initialize Image buffer ..." );
		return -1;
	}

	return 0;
}

int ImageRebuilder::Flush2ReqSessions( DatabaseIO& refDatabaseIO, unsigned __int64 nSerialNo )
{
	unsigned int		lstTableID[64] = { 0 };
	unsigned int		nTableCount = refDatabaseIO.GetTablesID( lstTableID, 64 );
	int					nSetSize = ImageRebuilder::GetObj().GetReqSessionCount();

	for( int n = 0; n < nTableCount && nSetSize > 0; n++ )
	{
		unsigned __int64	nQueryID = nSerialNo;
		unsigned int		nTableID = lstTableID[n];
		int					nDataLen = refDatabaseIO.FetchRecordsByID( nTableID, m_pImageDataBuffer, MAX_IMAGE_BUFFER_SIZE, nQueryID );

		if( nDataLen < 0 )
		{
			DataNodeService::GetSerivceObj().WriteWarning( "LinkSessions::OnNewLink() : failed 2 fetch image of table, errorcode=%d", nDataLen );
			return -1 * (n*100);
		}

		for( std::set<unsigned int>::iterator it = m_setNewReqLinkID.begin(); it != m_setNewReqLinkID.end(); it++ )
		{
			nDataLen = DataNodeService::GetSerivceObj().SendData( *it, 0, 0, m_pImageDataBuffer, nDataLen/*, nSerialNo*/ );
			if( nDataLen < 0 )
			{
				DataNodeService::GetSerivceObj().WriteWarning( "LinkSessions::OnNewLink() : failed 2 send image data, errorcode=%d", nDataLen );
				return -2 * (n*100);
			}
		}
	}

	for( std::set<unsigned int>::iterator it = m_setNewReqLinkID.begin(); it != m_setNewReqLinkID.end(); it++ )
	{
		LinkIDSet::GetSetObject().NewLinkID( *it );
	}

	m_setNewReqLinkID.clear();

	return nSetSize;
}

unsigned int ImageRebuilder::GetReqSessionCount()
{
	CriticalLock		lock( m_oBuffLock );

	return m_setNewReqLinkID.size();
}

bool ImageRebuilder::AddNewReqSession( unsigned int nLinkNo )
{
	CriticalLock		lock( m_oBuffLock );

	if( m_setNewReqLinkID.find( nLinkNo ) == m_setNewReqLinkID.end() )
	{
		DataNodeService::GetSerivceObj().WriteInfo( "ImageRebuilder::AddNewReqSession() : [WARNING] duplicate link number & new link will be disconnected..." );

		return false;
	}

	m_setNewReqLinkID.insert( nLinkNo );

	return true;
}







