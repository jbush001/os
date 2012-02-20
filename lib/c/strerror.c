#include "types.h"
#include "syscall.h"

#define ERROR_BASE -12

const char *error_text[] = {
	"Entry already exists",
	"Invalid Operation",
	"Not Directory",
	"No Such File",
	"IO Error",
	"Wait Collision",
	"Invalid Address",
	"Operation Not Permitted",
	"Bad Object Type",
	"Invalid Object IO",
	"Timed Out",
	"OS Error",
	"No Error"
};

const char* strerror(status_t error)
{
	if (error >= 0)
		return "No Error";
	
	if (error < ERROR_BASE)
		return "Unknown Error";
		
	return error_text[-ERROR_BASE + error];
}
