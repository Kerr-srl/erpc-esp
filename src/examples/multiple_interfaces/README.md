# Example with multiple interfaces

## Build and flash

```sh
idf.py build
# Or if you're using another family
idf.py -DIDF_TARGET="esp32s2" build

# Flash. 3M is almost the highest speed I was able to get. Consider lowering
# this value if flashing fails.
idf.py -b 3000000 flash
```
