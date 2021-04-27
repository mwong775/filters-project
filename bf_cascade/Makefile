CFLAGS = -pg -g -Wall -std=c++14 -mpopcnt -march=native

# all: morton bbf dbf bf vf cf vfss cfss
 
# morton: morton.cpp
# 	g++ $(CFLAGS) -Ofast -o morton morton.cpp 

bf : bf_test.cpp
	g++ $(CFLAGS) -Ofast -o bf_test bf_test.cpp 

# bbf: bbf.cpp
# 	g++ $(CFLAGS) -Ofast -o bbf bbf.cpp 

# dbf: dbf.cpp
# 	g++ $(CFLAGS) -Ofast -o dbf dbf.cpp 

# vf: vf.cpp
# 	g++ $(CFLAGS) -Ofast -o vf vf.cpp 

# cf: cf.cpp
# 	g++ $(CFLAGS) -Ofast -o cf cf.cpp 

# vfss: vfss.cpp
# 	g++ $(CFLAGS) -Ofast -o vfss vfss.cpp 

# cfss: cfss.cpp
# 	g++ $(CFLAGS) -Ofast -o cfss cfss.cpp 

clean:
	rm -f morton
	rm -f bbf
	rm -f dbf
	rm -f bf
	rm -f vf
	rm -f cf
	rm -f cfss
	rm -f vfss
