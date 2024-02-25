PROG = parentchildcomm
SRC = parentchildcomm.c

.PHONY: clean

$(PROG): $(SRC)
	gcc -g -o $@ $<

clean:
	rm -f $(PROG) *.o *~ 
