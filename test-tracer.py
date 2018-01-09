#!/usr/bin/env python
from pwn import *
import random
import os

process_path = './build-river-tools/bin/river.tracer'
test_lib_path = '/home/alex/libhttp-parser.so'
max_tests = 1000
max_len = 1024

error_messages = ["Disassembling unknown instruction", "Translating unknown instruction"]

execution_log = 'execution.log'

def find_text_in_file(text, filename):
    if text in open(filename).read():
        return True
    return False


def generate_test():
    len = random.randint(1, 1024)
    return ''.join(random.choice(string.ascii_uppercase + string.ascii_lowercase + string.digits) for _ in range(len))

if __name__ == "__main__":
    current_test = 0
    while current_test < max_tests:
        tracer_process_args = [process_path, '--annotated', '--z3', '-p', test_lib_path, '-o', 'trace.simple.out.' + str(current_test)]
        io = process(tracer_process_args)
        payload = generate_test()
        print("Send payload: [%s] of len: [%d]" % (payload, len(payload)))
        io.send(payload)
        io.stdin.close()
        while True:
            try:
                io.recv()
            except EOFError:
                break
        io.close()
        ## check if unk instruction was found in executin log
        for e in error_messages:
            if find_text_in_file(e, execution_log):
                print("Obtained error for payload: [%s] in test: [%d]" % (payload, current_test))
                break
        os.rename(execution_log, 'execution.log.' + str(current_test))
        current_test += 1
