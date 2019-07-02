FROM ubuntu:18.04
WORKDIR /libfuzzer_lpm_mutator
COPY src /libfuzzer_lpm_mutator
RUN  apt update && apt install -y sudo
