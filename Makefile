all:
	$(MAKE) -C pflayer all pf_benchmark
	$(MAKE) -C amlayer all am_student_benchmark
	$(MAKE) -C hflayer all hf_student_benchmark

experiments: all
	./run_all_experiments.sh

clean:
	$(MAKE) -C pflayer clean
	$(MAKE) -C amlayer clean
	$(MAKE) -C hflayer clean

.PHONY: all experiments clean
