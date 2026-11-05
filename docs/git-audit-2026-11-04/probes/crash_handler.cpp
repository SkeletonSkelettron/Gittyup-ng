#include <QCoreApplication>
#include <QDebug>
#include <cstring>
#include <stdexcept>
#include "CrashHandler.h"
int main(int argc, char **argv) {
  QCoreApplication app(argc,argv);
  CrashHandler::install(true);
  if(argc==1) return 0;
  if(!strcmp(argv[1],"warning")) qWarning("Review warning after handler install");
  if(!strcmp(argv[1],"fatal")) qFatal("Review fatal");
  if(!strcmp(argv[1],"throw")) throw std::runtime_error("Review exception");
  if(!strcmp(argv[1],"av")) *reinterpret_cast<volatile int *>(0)=1;
}
