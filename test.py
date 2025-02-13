# Test file with various lint issues

import sys
import os  # unused import
from collections import defaultdict, Counter  # Counter is unused

def unused_var_function():
    x = 5  # unused variable
    y = 10  # unused variable
    return 42

test_dict = {
    "a": 1,
    "b": 2,
    "a": 3,  # duplicate key
    "c": 4,
    "b": 5,  # duplicate key
    "d": {
        "nested": 1,
        "nested": 2  # duplicate nested key
    }
}

def main():
    result = unused_var_function()
    print(f"Result: {result}")
    print(f"Using sys.version: {sys.version}")
    
    a = 6 # used variable
    b = 7 # used variable
    c = a + b # unused variable


    word_counts = Counter(['apple', 'banana', 'apple', 'cherry', 'banana', 'apple'])
    print("Most common words:", word_counts.most_common(2))

    for k, v in test_dict.items():
        print(f"{k}: {v}")

if __name__ == "__main__":
    main()
