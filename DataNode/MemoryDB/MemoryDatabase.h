#ifndef __MEMORYDB_MEMORYDATABASE_H__
#define	__MEMORYDB_MEMORYDATABASE_H__


#include "Interface.h"
#include "../Infrastructure/Dll.h"


/**
 * @class							DatabaseIO
 * @brief							数据库管理类
 * @date							2017/5/4
 * @author							barry
 */
class DatabaseIO
{
public:
	DatabaseIO();
	~DatabaseIO();

	/**
	 * @brief						初始化数据库管理对象
	 * @return						==0				成功
									!=0				错误
	 */
	int								Initialize();

	/**
	 * @brief						释放所有资源
	 */
	void							Release();

protected:
	Dll								m_oDllPlugin;					///< 插件加载类
	IDBFactory*						m_pIDBFactoryPtr;				///< 工厂类
	I_Database*						m_pIDatabase;					///< 数据库指针
};



#endif









