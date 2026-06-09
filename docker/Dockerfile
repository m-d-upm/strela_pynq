FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y

WORKDIR /app

COPY ../src/umd/linux/strela_test_linux.elf /app/strela_test_linux.elf

RUN chmod +x /app/strela_test_linux.elf

CMD ["./strela_test_linux.elf"]
