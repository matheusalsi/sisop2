FROM gcc:latest
COPY . /usr/src/wakeonlan
WORKDIR /usr/src/wakeonlan
RUN make
CMD ["./wakeonlan.app"]
