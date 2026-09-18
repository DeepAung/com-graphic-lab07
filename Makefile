# Choose my "NVIDIA GeForce RTX 3060 Laptop GPU"
export MESA_VK_DEVICE_SELECT := 10de:2520

BUILD := build
PARTS := info circle terrain sdf heightmap toy

.PHONY: all clean $(PARTS)

all:
	cmake -S . -B $(BUILD) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD)

$(PARTS):
	cmake -S . -B $(BUILD) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD) --target $@
	@echo "running ./$(BUILD)/$@"
	@./$(BUILD)/$@ $(ARGS)

clean:
	rm -rf $(BUILD)
