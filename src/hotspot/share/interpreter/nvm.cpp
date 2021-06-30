#include "interpreter/nvm.hpp"
#include "oops/oop.inline.hpp"
#include "oops/klass.hpp"
#include "oops/access.inline.hpp"
#include "runtime/signature.hpp"
#include "utilities/globalDefinitions.hpp"

#define DEBUG_NVM 0

ManagementNVM nvm(NVM_SIZE);

std::mutex mtx_nvm;

void ManagementNVM::call_new_nvm(long int klass_size, long int *ad, bool is_static) {
  if(is_static) {
    if(*ad == 0 && mem_static+klass_size <= end_static) {
      char* tmp = mem_static;
      char* tmp_ad = (char*) ad;
      *ad = (long int)mem_static;
      mem_static = mem_static + klass_size;
      
      for(int i = 0; i < klass_size; i++) {
	*tmp = *tmp_ad;
	tmp++;
	tmp_ad++;
      }
    } else {
      printf("wrong case\n");
    }
  } else {
    if(*ad == 0 && mem+klass_size <= end) {
      char* tmp = mem;
      char* tmp_ad = (char*) ad;
      *ad = (long int)mem;
      mem = mem + klass_size;
      
      for(int i = 0; i < klass_size; i++) {
	*tmp = *tmp_ad;
	tmp++;
	tmp_ad++;
      }
    } else {
      printf("wrong case\n");
    }
  }
}

void ManagementNVM::debug_call_new_nvm(long int klass_size, long int *ad, bool is_static) {
  mtx_nvm.lock();
  if(is_static) {
    if(mem_static+klass_size <= end_static) {
      char* tmp = mem_static;
      char* tmp_ad = (char*) ad;
      *ad = (long int)mem_static;
      mem_static = mem_static + klass_size;
      
      for(int i = 0; i < klass_size; i++) {
	*tmp = *tmp_ad;
	tmp++;
	tmp_ad++;
      }
    } else {
      printf("debug wrong case\n");
    }
  } else {
    if(mem+klass_size <= end) {
      char* tmp = mem;
      char* tmp_ad = (char*) ad;
      *ad = (long int)mem;
      mem = mem + klass_size;
      
      for(int i = 0; i < klass_size; i++) {
	*tmp = *tmp_ad;
	tmp++;
	tmp_ad++;
      }
    } else {
      printf("debug wrong case\n");
    }
  }
  mtx_nvm.unlock();
}

void ManagementNVM::call_new(oopDesc* obj) {
  int klass_size = obj->klass()->layout_helper();
  call_new_nvm(klass_size, (long int*)obj, false);
}

void ManagementNVM::call_static(oopDesc* obj) {
  int klass_size = obj->klass()->layout_helper();
  call_new_nvm(klass_size, (long int*)obj, true);
}

//////////////////////////////////////////////////////////

void StaticNVM::make_nvm_space(oopDesc* obj, bool is_static) {
  int klass_size = obj->klass()->layout_helper();
  nvm.debug_call_new_nvm(klass_size, (long int*) obj, is_static);
}

void StaticNVM::call_new(oopDesc* obj){
  make_nvm_space(obj, false);
}

void StaticNVM::call_static(oopDesc* obj) {
  make_nvm_space(obj, true);
}

void StaticNVM::compare_obj_detail(oopDesc* obj) {
  int klass_size = obj->klass()->layout_helper();
  char *start_dram = (char*) obj;
  char *start_nvm = (char*)*((long int*)obj);
  printf("DRAM : ");
  for(int i = 0; i < klass_size; i++) {
    printf("%2x", *(start_dram+i));
  }
  printf("\n");
  printf("NVM  : ");
  for(int i = 0; i < klass_size; i++) {
    printf("%2x", *(start_nvm+i));
  }
  printf("\n");
  for(int i = 0; i < klass_size; i++) {
    if(*(start_dram+i) != *(start_nvm+i)) {
      
      printf("i is %d : wrong information : %x %x \n",i, *(start_dram+i), *(start_nvm+i));
      return;
    }
  }
  printf("correct information\n");
}

void StaticNVM::compare_obj(oopDesc* obj) {
  int klass_size = obj->klass()->layout_helper();
  char *start_dram = (char*) obj;
  char *start_nvm = (char*)(*((long int*)obj));

  auto klass_info = (InstanceKlass*) (obj->klass());
  auto field_count = klass_info->java_fields_count();
  for(int i = 0; i < field_count; i++) {
    // if(!strcmp(klass_info->field_name(i)->as_C_string(), "serialVersionUID")) return;
  }
  for(int i = sizeof(oopDesc); i < klass_size; i++) {
    if(*(start_dram+i) != *(start_nvm+i)) {
      printf("class name : %s\n", obj->klass()->name()->as_C_string());
      printf("i is %d : wrong information : %d %d \n",i, *(start_dram+i), *(start_nvm+i));
      
      printf("DRAM : ");
      for(int j = 0; j < klass_size; j++) {
	printf("%3d", *(start_dram+j));
      }
      printf("\n");
      printf("NVM  : ");
      for(int j = 0; j < klass_size; j++) {
	printf("%3d", *(start_nvm+j));
      }
      printf("\n");

      printf("field_count : %d\n", field_count);
      for(int j = 0; j < field_count; j++) {
	auto field_name = klass_info->field_name(j)->as_C_string();
	printf("%s ", field_name);
      }
      printf("\n");
      return;
    }
  }
    //  printf("correct information\n");
}

void StaticNVM::compare_only_new(oopDesc* obj){
#ifdef DEBUG_NVM
  if(!oopDesc::is_oop(obj)) return;
  if(obj == NULL || *(long int*) obj == 0) return;
  if((char*)(*(long int*) obj) < nvm.start || (char*)(*(long int*) obj) > nvm.end) return;
  compare_obj(obj);
#endif
}

void StaticNVM::deep_copy_nvm(oopDesc* obj) {
  mtx_nvm.lock();
  if(obj == NULL || *(long int*) obj == 0) {
    mtx_nvm.unlock();
    return;
  }
  int klass_size = obj->klass()->layout_helper();
  char *start_dram = (char*) obj;
  char *start_nvm = (char*)*((long int*)obj);
  if(nvm.start > start_nvm || start_nvm > nvm.end) {
    mtx_nvm.unlock();
    return;
  }
  for(int i = 0; i < klass_size; i++) {
    *(start_nvm+i) = *(start_dram+i);
  }
  mtx_nvm.unlock();
}

void StaticNVM::debug_make_nvm_space(long int klass_size, long int *ad) {
  nvm.debug_call_new_nvm(klass_size, ad, false);
}

void StaticNVM::compare_field(oopDesc* obj, int offset, int byte){
  if(obj == NULL || *(long int*) obj == 0) return;
  bool flag = true;
  switch(byte) {
  case 1:
    {
    char *start_dram = (char*) obj;
    char *start_nvm = (char*)*((long int*) obj);
    flag = (*start_dram == *start_nvm);
    if(!flag) printf("DRAM : %c , NVM : %c\n", *start_dram, *start_nvm);}
    break;
  case 4:
    {
    int *start_dram = (int*) obj;
    int *start_nvm = (int*)*((long int*) obj);
    flag = (*start_dram == *start_nvm);
    if(!flag) printf("DRAM : %d , NVM : %d\n", *start_dram, *start_nvm);
    }
    break;
  case 8:
    {
    long long int *start_dram = (long long int*) obj;
    long long int *start_nvm = (long long int*)*((long int*) obj);
    flag = (*start_dram == *start_nvm);
    if(!flag) printf("DRAM : %lld , NVM : %lld\n", *start_dram, *start_nvm);
    }
    break;
  default :
    {
    char *start_dram = (char*) obj;
    char *start_nvm = (char*)*((long int*) obj);
    
    for(int i = 0; i < byte; i++) {
      flag = (*(start_nvm+offset+i) == *(start_dram+offset+i)) ;
      if(!flag) printf("DRAM : %c , NVM : %c\n", *start_dram+offset+i, *start_nvm+offset+i);
      break;
    }
    }
    break;
  }
  if(!flag) exit(0);
}
