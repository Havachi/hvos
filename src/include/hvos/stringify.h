#ifndef	HVOS_STRINGIFY_H
#define HVOS_STRINGIFY_H

#define __stringify_1(x...)		#x
#define __stringify(x...) 		__stringify_1(x)

#define FILE_LINE				__FILE__ ":" __stringify(__LINE__)


#endif