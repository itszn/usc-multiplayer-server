.PHONY: build
build: 
	cd src && go build -o ../usc_multiplayer -v

.PHONY: doc
doc:
	docker run --rm \
		-v $(shell pwd)/doc:/out \
		-v $(shell pwd)/doc/proto:/protos \
		-v $(shell pwd)/doc/template:/template \
		pseudomuto/protoc-gen-doc --doc_opt=/template/html.tmpl,server.html Server.proto
	docker run --rm \
		-v $(shell pwd)/doc:/out \
		-v $(shell pwd)/doc/proto:/protos \
		-v $(shell pwd)/doc/template:/template \
		pseudomuto/protoc-gen-doc --doc_opt=/template/html.tmpl,client.html Client.proto Types.proto
