#!/usr/bin/env python3
"""Replace != nullptr / == nullptr with implicit bool in C++ core/ files."""
import re
import os


def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    original = content

    # Step 1: Replace != nullptr → (just remove "!= nullptr")
    content = re.sub(r'\s*!=\s*nullptr', '', content)

    # Step 2: Replace == nullptr → !(expr)
    # Process from right to left to avoid index invalidation
    matches = list(re.finditer(r'==\s*nullptr', content))
    for m in reversed(matches):
        match_start = m.start()
        match_end = m.end()

        # Walk backwards from match_start to find expression start
        j = match_start - 1
        # Skip whitespace
        while j >= 0 and content[j] in ' \t':
            j -= 1

        if j < 0:
            continue

        # Find expression start by walking backwards
        depth_paren = 0
        depth_bracket = 0
        depth_brace = 0
        expr_start = j + 1  # default: expression is single char at j

        while j >= 0:
            ch = content[j]
            if ch == ')':
                depth_paren += 1
            elif ch == '(':
                if depth_paren > 0:
                    depth_paren -= 1
                else:
                    expr_start = j + 1
                    break
            elif ch == ']':
                depth_bracket += 1
            elif ch == '[':
                if depth_bracket > 0:
                    depth_bracket -= 1
                else:
                    expr_start = j
                    break
            elif ch == '}':
                depth_brace += 1
            elif ch == '{':
                if depth_brace > 0:
                    depth_brace -= 1
                else:
                    expr_start = j + 1
                    break
            elif ch == '>' and j > 0 and content[j - 1] == '-':
                # Part of -> operator, skip both chars
                j -= 2
                continue
            elif depth_paren == 0 and depth_bracket == 0 and depth_brace == 0:
                if ch in ';:,?&|!^~><':
                    expr_start = j + 1
                    break
                elif ch in ' \t\n\r':
                    expr_start = j + 1
                    break
                # Stop at +/- that are binary operators (not unary)
                elif ch in '+-' and j > 0 and content[j - 1] not in '(,=:&|!^~;{[ \t\n\r':
                    expr_start = j + 1
                    break
            if j == 0:
                expr_start = 0
                break
            j -= 1

        expr = content[expr_start:match_start].rstrip()

        # Replace the entire expr == nullptr with !(expr)
        content = content[:expr_start] + '!' + expr + content[match_end:]

    if content != original:
        with open(filepath, 'w') as f:
            f.write(content)
        orig_lines = original.splitlines()
        new_lines = content.splitlines()
        changes = sum(1 for a, b in zip(orig_lines, new_lines) if a != b)
        print(f"  {os.path.basename(filepath)}: {changes} lines changed")
        return changes
    return 0


def main():
    total = 0
    core_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'core')
    files = sorted(
        os.path.join(core_dir, f)
        for f in os.listdir(core_dir)
        if f.endswith(('.cpp', '.h'))
    )
    for f in files:
        total += process_file(f)
    print(f"\nTotal: {total} lines changed")


if __name__ == '__main__':
    main()
