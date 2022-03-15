// include {{{
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
// }}}
// Macro definition {{{
#ifdef _DEBUG
  #define SAY_HELLO() \
    do {printf("[%s]: Hello from me!\n", __FUNCTION__);} while(0)
  #define DBG(fmt, ...) \
    do {printf("{%s}: " fmt "\n", __FUNCTION__, __VA_ARGS__);} while(0)

#else
  #define SAY_HELLO() {}
  #define DBG(fmt, ...) {}
#endif

#define MAX_NVRAM 94
#ifndef _NVRAM_FILE
  #define _NVRAM_FILE "/nvram/my_nvram"
#endif
#ifndef _DUMP_FILE
  #define _DUMP_FILE "/nvram/dump_nvram"
#endif
#ifndef _RESET_NVRAM
  #define _RESET_NVRAM "/etc/nvram"
#endif
// }}}
// Type definition {{{
typedef struct t_element{
  char name[256];
  char value[256];
  struct t_element *b_list_next;
}element;

element* nvram[MAX_NVRAM];
// }}}

element* find_elem(char *key){
  size_t offset;
  // First, we need to compute the offset
  // TODO check strlen(key)
  offset =  (int)key[0] - 0x21;
  //DBG("Offset for %s is %d", key, offset);
  if (nvram[offset] == NULL)
    return NULL;
    // Now we can walk in the bucket list
  element* next;
  for (next = nvram[offset]; next != NULL; next = next->b_list_next) {
    //DBG("Key %s Value %s", next->name, next->value);
    if(strcmp(next->name, key) == 0)
      return next;
    //DBG("Key: %s", next->name);
  }
  return NULL;
}
void insert_element(element* el) {
  //SAY_HELLO();
  size_t offset;
  offset  = (int)el->name[0] - 0x21;
  el->b_list_next = NULL;
  //DBG("Offset %d", offset);
  if (nvram[offset] == NULL) {
    nvram[offset] = el;
  }
  else{
    element* next;
    for(next = nvram[offset];next->b_list_next != NULL; next = next->b_list_next); 
    next->b_list_next = el;
  }
}
void add_or_sub(char *el){
  char *key, *value;
  char delim[] = "=";
  element* elem;
  int i=0;
  key = strtok(el, delim);  
  value = strtok(NULL, delim);  
  //DBG("%s -- > %s", key, value);
  // Compute index and list buckets
  elem = find_elem(key);
  // If element not exist, we will create one
  if (elem == NULL){
    elem = (element*)malloc(sizeof(element));
    strncpy(elem->name, key, 256);
    i=1;
  }
  if (value != NULL)
    strncpy(elem->value, value, 256);
  else
    strcpy(elem->value, "");
  if (i == 1)
    insert_element(elem);
}
int read_reset_file(int fd, size_t offset, int line_char) {
  //SAY_HELLO();
  size_t index;
  char line[512];
  // First, we will read the entire item
  if (read(fd, line, offset) != offset)
    return -1;
  DBG("I read 0x%x bytes", offset);
  // Clear the line
  memset((void*)line, 0x0, 512); 
  index = 0;
  while(1) {
    offset = read(fd, &line[index], 1);
    if(offset == 0)
      break;
    if(line[index] == line_char) {
      line[index] = 0x0;
      if (index == 0)
        continue;
      // Line completed
      //DBG("Readed %s", line);
      add_or_sub(line);
      memset((void*)line, 0x0, 512); 
      index = 0;
    }
    else
      index++;
  }
  return 0;
}

void _init(void) {
  SAY_HELLO();
  // Initialize the nvram vector
  int i,fd;
  for (i=0;i<MAX_NVRAM;i++)
    nvram[i] = NULL;
#define _RESET_START
#ifdef _RESET_START
  // Now, we can read the nvram structure
  // First, we will read the default value
  //fd = open(_RESET_NVRAM, O_RDONLY);
  //if (fd == -1) {
  //  fprintf(stderr, "[%s]: File "_RESET_NVRAM " can not be opened!\n", __FUNCTION__);
  //  return;
  //}
  //// Read reset_nvram
  //i=read_reset_file(fd, 0x20, 0x0);
  //close(fd);
  fd = open(_NVRAM_FILE, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "[%s]: File "_NVRAM_FILE " can not be opened!\n", __FUNCTION__);
    return;
  }
  i = read_reset_file(fd, 0x0, 0x0a);
  close(fd);
#else 
  fd = open(_DUMP_FILE, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "[%s]: File "_NVRAM_FILE " can not be opened!\n", __FUNCTION__);
    return;
  }
  i = read_reset_file(fd, 0x0, 0x0a);
  close(fd);
#endif
  
}

void _fini(void) {
  SAY_HELLO();
}

/**
 * This function allows to check if the KEY option is set to VALUE.
 * 
 * @param[in] key    Key option to search in the NVRAM
 * @param[in] value  Value to compare
 * @return       (nvram[key] == value) 1 : 0
 **/
int acosNvramConfig_match(char *key, char *value) {
  element *e;
  if (strcmp("ntgr_cgi_debug_msg", key) == 0)
    return 1;
  DBG("nvram[%s] == %s ?", key, value);
  e = find_elem(key);
  if (e == NULL || value == NULL || strncmp(e->value, value, 256) != 0) {
    DBG("-> 0, element is %s", (e == NULL) ? "NULL" : e->value); 
    return 0;
  }
  DBG("-> 1", 1);
  return 1;
}
/**
 * This function allows to check if the KEY option is set to VALUE.
 * 
 * @param[in] key    Key option to search in the NVRAM
 * @param[in] value  Value to compare
 * @return       (nvram[key] == value) 0 : 1
 **/
int acosNvramConfig_invmatch(char *key, char *value) {
  element *e;
  DBG("nvram[%s] == %s ?", key, value);
  e = find_elem(key);
  if (e == NULL)
    return 0;
  if (strncmp(e->value, value, 256) == 0) {
    DBG("-> 0, element is %s", (e == NULL) ? "NULL" : e->value); 
    return 0;
  }
  DBG("-> 1", 1);
  return 1;
}

/**
 * This function is used to read the KEY option from the NVRAM. 
 *
 * The value is copied to DST param
 * 
 * @param[in] key    Key option to search in the NVRAM
 * @param[out] dst   Value to compare
 * @return           (nvram[key] != NULL) 0 : 1
 **/
int acosNvramConfig_read(char *key, char *dst, size_t size){
  element *e;
  DBG("nvram[%s]", key);
  e = find_elem(key);
  if (e == NULL)
    return 1;
  DBG("nvram[%s] = %s", key, e->value);
  strncpy(dst, e->value, size);
  return 0;
}

/**
 * This function return the value of KEY
 *
 * @param[in] key    Key option to search in the NVRAM
 * @return           strdup of value
 **/
char *acosNvramConfig_get(char *key) {
  element *e;
  DBG("nvram[%s]", key);
  e = find_elem(key);
  if (e == NULL)
    return NULL;
  DBG("nvram[%s] = %s", key, e->value);
  return strdup(e->value);
}
/**
 * This function return the value of atoi(nvram[KEY])
 *
 * @param[in]  key    Key option to search in the NVRAM
 * @param[out] value  Returned value
 * @return            0 if ok, 1 elsewhere
 **/
int acosNvramConfig_readAsInt(char *key, int* value) {
  element *e;
  DBG("nvram[%s]", key);
  e = find_elem(key);
  if (e == NULL)
    return 0;
  DBG("nvram[%s] = %s", key, e->value);
  (*value) = atoi(e->value);
  return 0;
}

/**
 * This function return the value of KEY
 *
 * @param[in] key    Key option to search in the NVRAM
 * @param[in] value  Option value
 **/
int acosNvramConfig_set(char *key, char *value) {
  element *e;
  DBG("nvram[%s]=%s", key, value);
  e = find_elem(key);
  if (e == NULL) {
    e = (element*)malloc(sizeof(element));
    strncpy(e->name, key, 256);
    insert_element(e);
  }
  if (value != NULL)
    strncpy(e->value, value, 256);
  return -1;
}

/**   
 * This function is useless for us but we can use to consolidate the NVRAM status to file
 * 
 **/
void acosNvramConfig_save(void) {
  // The real nvram library use this function for commit change to NVRAM
  // We can dump the entire nvram contents to a file
  element *el;
  char line[514];
  int index;
  int fd = open(_DUMP_FILE, O_WRONLY | O_CREAT);
  if (fd == -1){
    DBG("Errore for dumping file %s", _DUMP_FILE);
    return;
  }
  // Now we should walk inside the nvram
  for(index=0; index<MAX_NVRAM; index++){
    for (el = nvram[index]; el != NULL; el=el->b_list_next){
      memset(line, 0, 514);
      strncpy(line, el->name, 256);
      strncat(line, "=", 2);
      strncat(line, el->value, 256);
      strncat(line, "\n", 2);
      //DBG("Write Line %s", line);
      write(fd, line, strnlen(line, 514));
    }
  }
  close(fd);
}


int WAN_ith_CONFIG_GET(int ith, char *format, char *dst) {
  char line[0x1ff];
  element *el;
  memset(line, 0x0, 0x1ff);
  DBG("Format string %s, ith: %d", format, ith);
  sprintf(line, format, ith);
  DBG("Search %s", line);
  el=find_elem(line);
  if (el == NULL)
    return 0;
  strcpy(dst, el->value);
  return 1;
}
// This will return 1 if config exist, 0 elsewhere
int acosNvramConfig_exist(char *key) {
  DBG("Check for %s", key);
  return ((find_elem(key) == NULL) ? 0 : 1);
}


int acosNvramConfig_save_config(void) {
  char key[] = "is_default";
  acosNvramConfig_set(key, "0");
  return 0;
}
int acosNvramConfig_unset() { 
  return 1;
}
