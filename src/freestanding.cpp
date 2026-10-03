#include <stddef.h>
extern "C" {
void* memcpy(void*to,const void*from,size_t n){auto*d=(unsigned char*)to;const auto*s=(const unsigned char*)from;for(size_t i=0;i<n;i++)d[i]=s[i];return to;}
void* memset(void*to,int v,size_t n){auto*d=(unsigned char*)to;for(size_t i=0;i<n;i++)d[i]=(unsigned char)v;return to;}
void* memmove(void*to,const void*from,size_t n){auto*d=(unsigned char*)to;const auto*s=(const unsigned char*)from;if(d<s){for(size_t i=0;i<n;i++)d[i]=s[i];}else{while(n){--n;d[n]=s[n];}}return to;}
int memcmp(const void*a,const void*b,size_t n){auto*x=(const unsigned char*)a;auto*y=(const unsigned char*)b;for(size_t i=0;i<n;i++)if(x[i]!=y[i])return (int)x[i]-y[i];return 0;}
size_t strlen(const char*p){size_t n=0;while(p[n])n++;return n;}
}
