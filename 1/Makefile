.PHONY: gentest, clean, run
gentest:
	python3 generator.py -f test1.txt -c 3 -m 10
clean:
	rm test* && rm *.out
a.out:
	gcc seq.c
run:
	./a.out