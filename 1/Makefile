.PHONY: test, clean
test:
	python3 generator.py -f test1.txt -c 10000 -m 10000
	python3 generator.py -f test2.txt -c 10000 -m 10000
	python3 generator.py -f test3.txt -c 10000 -m 10000
	python3 generator.py -f test4.txt -c 10000 -m 10000
	python3 generator.py -f test5.txt -c 10000 -m 10000
	python3 generator.py -f test6.txt -c 10000 -m 10000
	python3 generator.py -f test7.txt -c 10000 -m 10000
	python3 generator.py -f test8.txt -c 10000 -m 10000
	python3 generator.py -f test9.txt -c 10000 -m 10000
	python3 generator.py -f test10.txt -c 10000 -m 10000
	gcc seq.c
	./a.out test1.txt test2.txt test3.txt test4.txt test5.txt test6.txt test7.txt test8.txt test9.txt test10.txt
	python3 checker.py -f output.txt
	make clean
clean:
	rm test* && rm *.out && rm output.txt
