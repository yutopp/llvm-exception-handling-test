g++ -Wall -std=c++11 -c eh_runtime.cpp && \
g++ -Wall -std=c++11 -c rill_runtime_exception.cpp && \
llc sample_0.ll && \
g++ eh_runtime.o rill_runtime_exception.o sample_0.s 
