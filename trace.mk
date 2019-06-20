TRACE=trace
$(TRACE)_CSRCS=trace.c
$(TRACE)_CXXSRCS=
$(TRACE)_OBJS=$(addsuffix .$(TRACE).o,$($(TRACE)_CSRCS) $($(TRACE)_CXXSRCS))
$(TRACE)_DEPS=$(addsuffix .$(TRACE).d,$($(TRACE)_CSRCS) $($(TRACE)_CXXSRCS))
$(TRACE)_CFLAGS=$(CFLAGS)
$(TRACE)_CXXFLAGS=$(CXXFLAGS)
$(TRACE)_LDFLAGS=$(LDFLAGS)

clean_$(TRACE):
	$(call pprintf,"CLEAN",$(TRACE))
	$(Q)rm -f $($(TRACE)_OBJS) $($(TRACE)_DEPS)
