import sys

alfabet = "abcdefghijklmnopqrstuvwxyz"

def generateFile(size: int, filename: str):
    size = size // len(alfabet)
    with open(filename, 'w') as f:
        f.write(alfabet*size + alfabet[:size] + '\n')


if __name__ == '__main__':
    generateFile(int(sys.argv[1]), sys.argv[2])
    print(f"File size {sys.argv[2]} generated")



