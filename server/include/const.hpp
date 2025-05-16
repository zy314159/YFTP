#pragma once
#define MAX_LENGTH  1024*2
//头部总长度
#define HEAD_TOTAL_LEN 4
//头部id长度
#define HEAD_ID_LEN 2
//头部数据长度
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000


enum MSG_IDS {
	MSG_HELLO_WORD = 1001,
    MSG_MKDIR, // 创建目录指令
    MSG_RMDIR, // 删除目录
    MSG_LIST,  // 列出目录内容
    MSG_UPLOAD, // 上传文件
    MSG_DOWNLOAD, // 下载文件
    MSG_CD, // 切换目录
    MSG_PWD, // 显示当前目录
};