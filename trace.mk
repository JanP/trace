TRACE=trace
$(TRACE)_CSRCS=trace.c
$(TRACE)_CXXSRCS=
$(TRACE)_STRACE_OBJS:=$(wildcard strace/strace-*.o strace/libstrace_a-*.o)
$(TRACE)_OBJS=$(addsuffix .$(TRACE).o,$($(TRACE)_CSRCS) $($(TRACE)_CXXSRCS)) $($(TRACE)_STRACE_OBJS)
$(TRACE)_DEPS=$(addsuffix .$(TRACE).d,$($(TRACE)_CSRCS) $($(TRACE)_CXXSRCS))
$(TRACE)_CFLAGS=$(CFLAGS) -I./strace -I./strace/linux -I./strace/linux/x86_64
$(TRACE)_CXXFLAGS=$(CXXFLAGS)
$(TRACE)_LDFLAGS=$(LDFLAGS) -ldw -lrt

strace/%.o:
	cd strace && ./bootstrace
	cd strace && ./configure --disable-mpers
	cd strace && make

clean_$(TRACE):
	$(call pprintf,"CLEAN",$(TRACE))
	$(Q)rm -f $($(TRACE)_OBJS) $($(TRACE)_DEPS)
