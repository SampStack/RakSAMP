BUILD_DIR ?= build
CONFIGURATION ?= Release
IMAGE_TAG ?= dev

.PHONY: configure build test image clean

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CONFIGURATION) -DBUILD_TESTING=ON

build: configure
	cmake --build $(BUILD_DIR) --config $(CONFIGURATION) --parallel

test: build
	ctest --test-dir $(BUILD_DIR) -C $(CONFIGURATION) --output-on-failure

image:
	docker build --target client --tag raksamp-client:$(IMAGE_TAG) .
	docker build --target server --tag raksamp-server:$(IMAGE_TAG) .

clean:
	cmake -E remove_directory $(BUILD_DIR)
