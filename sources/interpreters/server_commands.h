#ifndef SV_CMD
#define SV_CMD

//Server - Store commands
#define CMD_SS_EXIT 5000
#define CMD_SS_CREATE_BUCKET 5001
#define CMD_SS_UPLOAD_FILE 5002
#define CMD_SS_LIST_BUCKET 5003
#define CMD_SS_DOWNLOAD_FILE 5004
#define CMD_SS_REMOVE_FILE 5005
#define CMD_SS_PURGE_BUCKET 5006
#define CMD_SS_REQUEST_SIZE 5007
#define CMD_SS_UNDEFINED 5999

//Store - Server responses
#define RSP_SS_SYSTEM_ERROR 6000
#define RSP_SS_SUCCES 6001
#define RSP_SS_FILE_ALREADY_EXISTS 6004
#define RSP_SS_CLOSE_CONN 6998
#define RSP_SS_NOT_EXECUTED 6999

#endif