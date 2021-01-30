.PHONY: build
build: 
	cd src && go build -o ../usc_multiplayer -v
.PHONY: debug
debug: 
	cd src && go build -tags debug -o ../usc_multiplayer -v
.PHONY: get
get:
	cd src && go get

.PHONY: docs
docs:
	docker run --rm \
		-v $(shell pwd)/docs:/out \
		-v $(shell pwd)/docs/proto:/protos \
		-v $(shell pwd)/docs/template:/template \
		pseudomuto/protoc-gen-doc --doc_opt=/template/html.tmpl,server.html Server.proto
	docker run --rm \
		-v $(shell pwd)/docs:/out \
		-v $(shell pwd)/docs/proto:/protos \
		-v $(shell pwd)/docs/template:/template \
		pseudomuto/protoc-gen-doc --doc_opt=/template/html.tmpl,client.html Client.proto Types.proto
