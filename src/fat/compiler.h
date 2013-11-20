
#ifndef COMPILER_H_
#define COMPILER_H_

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(a) (a=a)
#endif


#ifdef __GNUC__
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
	#define INTERRUPT __attribute__ ((irq))
	#define PACKED_ARM 
	#define PACKED_GCC __attribute__ ((packed))
  
  #define PACKED_PRE  PACKED_ARM
  #define PACKED_PST  PACKED_GCC
  
#else
//	#error "This project is Gnu Compiler only!"
// #pragma pack
#define inline __inline
#define PACKED_ARM 
#define PACKED_GCC  
#define PACKED_PRE 
#define PACKED_PST 
#endif

#endif /*COMPILER_H_*/
