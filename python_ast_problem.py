import ast
import sys
from collections import defaultdict
from dataclasses import dataclass


@dataclass
class LintError:
    rule_name: str
    message: str
    line_number: int


class BaseLintRule(ast.NodeVisitor):
    def __init__(self, rule_name: str) -> None:
        self.errors: list[LintError] = []
        self.rule_name: str = rule_name


class UnusedImportsRule(BaseLintRule):
    def __init__(self, rule_name: str) -> None:
        super().__init__(rule_name)
        self.imports = []  # list of {'name': str, 'lineno': int}
        # set of used names
        self.used_names = set()

    def visit_Import(self, node: ast.Import) -> None:
        for alias in node.names:
            if alias.asname:
                imported_name = alias.asname
            else:
                imported_name = alias.name.split('.')[0]
            self.imports.append({'name': imported_name, 'lineno': node.lineno})
        self.generic_visit(node)

    def visit_ImportFrom(self, node: ast.ImportFrom) -> None:
        for alias in node.names:
            imported_name = alias.asname if alias.asname else alias.name
            self.imports.append({'name': imported_name, 'lineno': node.lineno})
        self.generic_visit(node)

    def visit_Name(self, node: ast.Name) -> None:
        if isinstance(node.ctx, ast.Load):
            self.used_names.add(node.id)
        self.generic_visit(node)

    def check(self, tree: ast.AST) -> list[LintError]:
        self.errors.clear()
        self.imports = []
        self.used_names = set()
        self.visit(tree)
        for imported in self.imports:
            if imported['name'] not in self.used_names:
                self.errors.append(LintError(
                    rule_name=self.rule_name,
                    message=f'Imported name "{imported["name"]}" is unused',
                    line_number=imported['lineno']
                ))
        return self.errors


class UnusedVariablesRule(BaseLintRule):
    pass


class DuplicateDictKeysRule(BaseLintRule):
    def visit_Dict(self, node: ast.Dict) -> None:
        seen = defaultdict(list)
        for key, value in zip(node.keys, node.values):
            if isinstance(key, ast.Constant):
                metadata = {"lineno": key.lineno, "key": key.value}
                seen[key.value].append(metadata)

            if isinstance(value, ast.Dict):
                self.visit(value)

        for key, metadata in seen.items():
            if len(metadata) > 1:
                lines = ", ".join(str(line["lineno"]) for line in metadata)
                self.errors.append(
                    LintError(
                        rule_name=self.rule_name,
                        message=f'Key "{key}" has been repeated on lines {lines}',
                        line_number=metadata[0]["lineno"],
                    )
                )

    def check(self, tree: ast.AST) -> list[LintError]:
        self.visit(tree)
        return self.errors


class Linter:
    def __init__(self) -> None:
        self.rules = [
            UnusedImportsRule(rule_name="unused_import"),
            # UnusedVariablesRule(rule_name="unused_variable"),
            DuplicateDictKeysRule(rule_name="duplicate_dict_keys"),
        ]

    def lint(self, source_code: str) -> list[LintError]:
        tree = ast.parse(source_code)
        errors: list[LintError] = []

        for rule in self.rules:
            errors.extend(rule.check(tree))

        return sorted(errors, key=lambda e: e.line_number)
    

if __name__ == "__main__":
    linter = Linter()
    with open("test.py", "r") as file:
        source_code = file.read()
    errors = linter.lint(source_code)
    for error in errors:
        print(error)
