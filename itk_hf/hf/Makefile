# C++ FLAGS
CC=g++
CFLAGS=-Wall -fPIC -O3 -D_FILE_OFSET_BITS=64 -I$(CUDA_INC_PATH) -I../include
LIBS=-pthread
LDFLAGS=-shared -fPIC $(LIBS) $(CUDA_LIBS)

SOURCES=global.cpp					\
	thread.cpp						\
	ComputingResource.cpp 			\
	Data.cpp	 					\
	ExecutionEngine.cpp 			\
	Flow.cpp	 					\
	MemoryAllocator.cpp				\
	Scheduler.cpp 					\
	TaskOrientedModule.cpp 			\
	TaskImplementation.cpp			\
	ThreadVPE.cpp					\
	VirtualProcessingElement.cpp 	\
	trace.cpp	

CUDA_SOURCES=

ifneq ($(TARGET),clean)

USE_PAPI=no
ifeq (${USE_PAPI},yes)
CFLAGS+= -DUSE_PAPI
LDFLAGS+= -lpapi
endif

# uncomment these to use kotf
HAVE_KOTF=0 
ifeq (${HAVE_KOTF},yes)
CFLAGS+= -DTRACE_ENABLED
LDFLAGS+= -lkotf
endif

HAVE_CUDA=0
CFLAGS+= -DHAVE_CUDA=${HAVE_CUDA}
ifeq (${HAVE_CUDA},1)
SOURCES+=	GPUDeviceVPE.cpp
else
CUDA_SOURCES=
CUDA_LIBS=
endif

endif

LIBRARY=libhf.so
OBJECTS=$(SOURCES:.cpp=.o) $(CUDA_SOURCES:.cu=.o)
DEPS=$(OBJECTS:.o=.d)
.SUFFIXES: .cu .cuh

all: $(SOURCES) $(LIBRARY)

clean:
	@rm -f $(LIBRARY)
	@rm -f *.o
	@rm -f *.d
	@rm -f *.d.*

install: $(LIBRARY)
	cp $(LIBRARY) ../lib/.

.cu.o:
	$(NVCC) $(NVCCFLAGS) $(NVCCINCLUDE) -c -o $@ $<

.cpp.o:
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIBRARY): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(LIBRARY)

define create-deps
	@set -e; rm -f $@; \
    $(CC) -x c++ -MM $(CFLAGS) $< > $@.$$$$; \
    sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
    rm -f $@.$$$$
endef

$(DEPS): $(SOURCES) $(CUDA_SOURCES)
	$(create-deps)

ifneq ($(TARGET),clean)
sinclude $(DEPS)
endif
