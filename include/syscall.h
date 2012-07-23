// 
// Copyright 1998-2012 Jeff Bush
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 

#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

int think();

int atomic_add(volatile int*, int);
int atomic_or(volatile int*, int);
int atomic_and(volatile int*, int);

/* Objects */
int close_handle(int);

/* Threads */
int sleep(bigtime_t time);
bigtime_t system_time();
int spawn_thread(thread_start_t, const char *name, void *data, int priority);
void thread_exit();
status_t exec(const char *path);
status_t wait_for_multiple_objects(int handleCount, const object_id *handles, bigtime_t timeout,
	WaitFlags flags);
status_t kill_thread(int thread_id);

/* Semaphores */
int create_sem(const char *name, int count);
status_t acquire_sem(int sem, bigtime_t timeout);
status_t release_sem(int sem, int count);

/* Areas */
int create_area(const char *name, void **addr, int searchFlags, unsigned int size,
	AreaWiring wiring, PageProtection protection);
int clone_area(const char *name, void **clone_addr, int searchFlags,
	PageProtection protection, int sourceArea);
int delete_area(int area_id);
int resize_area(int area_id, unsigned int newSize);

int _serial_print(const char *string);

/* Files */
int open(const char *pathname, int oflags);
status_t stat(const char *path, struct stat *out_stat);
status_t ioctl(int fd, int op, void *buffer);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t write_pos(int fd, off_t pos, const void *buf, size_t count);
ssize_t read(int fd, void *buf, size_t count);
ssize_t read_pos(int fd, off_t pos, void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);
status_t mkdir(const char *path, mode_t mode);
status_t rmdir(const char *path);
status_t rename(const char *old_name, const char *new_name);
status_t readdir(int fd, char *out_leafName, size_t inSize);
status_t rewinddir(int dirfd);
status_t chdir(const char *dir);
char *getcwd(char *buf, size_t size);
status_t mount(const char *device, const char *dir, const char *type,
	int rwflag, char *data);	
int map_file(const char *path, unsigned int va, off_t offset, size_t size, int flags);

	
const char* strerror(status_t error);
	
#ifdef __cplusplus
}
#endif



#endif
