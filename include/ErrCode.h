#ifndef _ERROR_CODE_H_A4729T84_3223_58276_5692_AV9FK38CMS9
#define _ERROR_CODE_H_A4729T84_3223_58276_5692_AV9FK38CMS9

//////////////////////////////////////////////////////////////
// Error code definition
//////////////////////////////////////////////////////////////

//** Function execution is success. **/
#ifndef TP_SUCCESS
#define TP_SUCCESS							0x0000
#endif

/** This function is not support. **/
#ifndef TP_ERR_COMMAND_NOT_SUPPORT
#define TP_ERR_COMMAND_NOT_SUPPORT  		0x0001
#endif

/** The touch ic may be occupied by other application **/
#ifndef TP_ERR_DEVICE_BUSY
#define TP_ERR_DEVICE_BUSY					0x0002
#endif

/** Read/Write data timeout **/
#ifndef TP_ERR_TIMEOUT
#define TP_ERR_TIMEOUT						0x0003
#endif

/** For asynchorous call, the execution is not finish yet. Waitting for complete **/
#ifndef TP_ERR_IO_PENDING
#define TP_ERR_IO_PENDING					0x0004
#endif

/** For any read command, sometimes the return data should meet some rule. **/
#ifndef TP_ERR_DATA_PATTERN
#define TP_ERR_DATA_PATTERN					0x0005
#endif

/** No interface create **/
#ifndef TP_ERR_NO_INTERFACE_CREATE
#define TP_ERR_NO_INTERFACE_CREATE			0x0006
#endif

#ifndef TP_ERR_HID_NOT_SUPPORT
#define TP_ERR_HID_NOT_SUPPORT				0x0007
#endif

#ifndef TP_ERR_INVALID_PARAM
#define TP_ERR_INVALID_PARAM				0x0008
#endif // TP_ERR_INVALID_PARAM

#ifndef TP_ERR_IO_ERROR
#define TP_ERR_IO_ERROR						0x0009
#endif // TP_ERR_IO_ERROR

/** Connect Elan Bridge and not get hello packet **/
#ifndef TP_ERR_CONNECT_NO_HELLO_PACKET
#define TP_ERR_CONNECT_NO_HELLO_PACKET		0x0102
#endif

/** Did not find any support device connectted to PC. Please check connectoin. **/
#ifndef TP_ERR_NOT_FOUND_DEVICE
#define TP_ERR_NOT_FOUND_DEVICE				0x0104
#endif

/** Someting Like File or Directory Not Found  **/
#ifndef TP_ERR_FILE_NOT_FOUND
#define TP_ERR_FILE_NOT_FOUND				0x0105
#endif //TP_ERR_FILE_NOT_FOUND

/* Unknown Device Type */
#ifndef TP_UNKNOWN_DEVICE_TYPE
#define TP_UNKNOWN_DEVICE_TYPE				0x010f
#endif // TP_UNKNOWN_DEVICE_TYP

#ifndef TP_GET_DATA_FAIL
#define	TP_GET_DATA_FAIL					0x0202
#endif // TP_GET_DATA_FAIL

/** Test Mode Error Code **/
#ifndef TP_TESTMODE_GET_RAWDATA_FAIL
#define TP_TESTMODE_GET_RAWDATA_FAIL		0x0301
#endif

/** CSV Parameter Error Code **/
#ifndef ERR_CSVFILE_NOT_EXIST
#define ERR_CSVFILE_NOT_EXIST				0x0401
#endif

#ifndef ERR_CSVFILE_OPEN_FAILURE
#define ERR_CSVFILE_OPEN_FAILURE			0x0402
#endif

#ifndef ERR_PARAM_FILE_INFO_INVALID
#define ERR_PARAM_FILE_INFO_INVALID			0x0403
#endif

#ifndef ERR_INIFILE_OPEN_FAILURE
#define ERR_INIFILE_OPEN_FAILURE			0x0501
#endif

#ifndef ERR_EEPROM_WRITE_FAILURE
#define ERR_EEPROM_WRITE_FAILURE			0x0601
#endif

/** Error information check, use GetErrMsg to get the error message. **/
#ifndef TP_ERR_CHK_MSG
#define TP_ERR_CHK_MSG						0xFFFF
#endif

#endif //_ERROR_CODE_H_A4729T84_3223_58276_5692_AV9FK38CMS9
