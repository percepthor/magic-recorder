TARGET      := magic-recorder

PTHREAD 	:= -l pthread
MATH 		:= -lm

# OPENCV 		:= -l opencv_core -l opencv_imgcodecs -l opencv_highgui -l opencv_shape -l opencv_videoio -l opencv_imgproc
OPENCV 		:= `pkg-config --cflags --libs opencv`

MAGIC_DEBUG		:= -D MAGIC_DEBUG

DEVELOPMENT		:= -g $(MAGIC_DEBUG)

CC          := g++

SRCDIR      := src
INCDIR      := include

BUILDDIR    := objs
TARGETDIR   := bin

CEXT		:= c
CPPEXT		:= cpp

DEPEXT      := d
OBJEXT      := o

CFLAGS      := $(DEVELOPMENT) $(DEFINES) -Wall -Wno-unknown-pragmas -Wfatal-errors
LIB         := $(PTHREAD) $(MATH) $(OPENCV)

INC         := -I $(INCDIR) -I /usr/local/include
INCDEP      := -I $(INCDIR)

CSOURCES	:= $(shell find $(SRCDIR) -type f -name *.$(CEXT))
CPPSORUCES	:= $(shell find $(SRCDIR) -type f -name *.$(CPPEXT))

SOURCES     := $(CSOURCES) $(CPPSORUCES)

COBJS		:= $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(CSOURCES:.$(CEXT)=.$(OBJEXT)))
CPPOBJS		:= $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(CPPSORUCES:.$(CPPEXT)=.$(OBJEXT)))

OBJECTS		:= $(COBJS) $(CPPOBJS)

all: directories $(TARGET)

run:
	./$(TARGETDIR)/$(TARGET)

directories:
	@mkdir -p $(TARGETDIR)
	@mkdir -p $(BUILDDIR)

clean:
	@$(RM) -rf $(BUILDDIR) 
	@$(RM) -rf $(TARGETDIR)

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.$(OBJEXT)=.$(DEPEXT))

# link
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(INC) $^ $(LIB) -o $(TARGETDIR)/$(TARGET)

# compile
$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(CEXT)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) $(LIB) -c -o $@ $<
	@$(CC) $(CFLAGS) $(INCDEP) -MM $(SRCDIR)/$*.$(CEXT) > $(BUILDDIR)/$*.$(DEPEXT)
	@cp -f $(BUILDDIR)/$*.$(DEPEXT) $(BUILDDIR)/$*.$(DEPEXT).tmp
	@sed -e 's|.*:|$(BUILDDIR)/$*.$(OBJEXT):|' < $(BUILDDIR)/$*.$(DEPEXT).tmp > $(BUILDDIR)/$*.$(DEPEXT)
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILDDIR)/$*.$(DEPEXT).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILDDIR)/$*.$(DEPEXT)
	@rm -f $(BUILDDIR)/$*.$(DEPEXT).tmp

$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(CPPEXT)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) $(LIB) -c -o $@ $<
	@$(CC) $(CFLAGS) $(INCDEP) -MM $(SRCDIR)/$*.$(CPPEXT) > $(BUILDDIR)/$*.$(DEPEXT)
	@cp -f $(BUILDDIR)/$*.$(DEPEXT) $(BUILDDIR)/$*.$(DEPEXT).tmp
	@sed -e 's|.*:|$(BUILDDIR)/$*.$(OBJEXT):|' < $(BUILDDIR)/$*.$(DEPEXT).tmp > $(BUILDDIR)/$*.$(DEPEXT)
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILDDIR)/$*.$(DEPEXT).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILDDIR)/$*.$(DEPEXT)
	@rm -f $(BUILDDIR)/$*.$(DEPEXT).tmp

.PHONY: all clean