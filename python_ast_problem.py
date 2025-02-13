import ast
import builtins
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
    def __init__(self, rule_name: str) -> None:
        super().__init__(rule_name)
        self.scopes = []
        self.builtin_names = set(dir(builtins))

    def _is_ignored(self, name: str) -> bool:
        return name.startswith('_') or name in self.builtin_names

    def _enter_scope(self) -> None:
        self.scopes.append({'defs': {}, 'used': set()})

    def _exit_scope(self) -> None:
        current_scope = self.scopes.pop()
        for var_name, lineno in current_scope['defs'].items():
            if var_name not in current_scope['used']:
                self.errors.append(LintError(
                    rule_name=self.rule_name,
                    message=f'Variable "{var_name}" is unused',
                    line_number=lineno
                ))

    def _record_variable(self, name: str, lineno: int) -> None:
        if not self._is_ignored(name):
            if self.scopes:
                current_scope = self.scopes[-1]
                if name not in current_scope['defs']:
                    current_scope['defs'][name] = lineno

    def _handle_assignment_target(self, target: ast.AST) -> None:
        if isinstance(target, ast.Name):
            self._record_variable(target.id, target.lineno)
        elif isinstance(target, (ast.Tuple, ast.List)):
            for elt in target.elts:
                self._handle_assignment_target(elt)

    def visit_FunctionDef(self, node: ast.FunctionDef) -> None:
        if not self._is_ignored(node.name):
            if self.scopes:
                enclosing_scope = self.scopes[-1]
                if node.name not in enclosing_scope['defs']:
                    enclosing_scope['defs'][node.name] = node.lineno
            else:
                self._enter_scope()
                self.scopes[-1]['defs'][node.name] = node.lineno
                self._exit_scope()
        self._enter_scope()
        args = node.args
        for arg in args.posonlyargs:
            self._record_variable(arg.arg, arg.lineno)
        for arg in args.args:
            self._record_variable(arg.arg, arg.lineno)
        for arg in args.kwonlyargs:
            self._record_variable(arg.arg, arg.lineno)
        if args.vararg:
            self._record_variable(args.vararg.arg, args.vararg.lineno)
        if args.kwarg:
            self._record_variable(args.kwarg.arg, args.kwarg.lineno)
        self.generic_visit(node)
        self._exit_scope()

    def visit_AsyncFunctionDef(self, node: ast.AsyncFunctionDef) -> None:
        self.visit_FunctionDef(node)

    def visit_ClassDef(self, node: ast.ClassDef) -> None:
        if not self._is_ignored(node.name):
            if self.scopes:
                enclosing_scope = self.scopes[-1]
                if node.name not in enclosing_scope['defs']:
                    enclosing_scope['defs'][node.name] = node.lineno
            else:
                self._enter_scope()
                self.scopes[-1]['defs'][node.name] = node.lineno
                self._exit_scope()
        self._enter_scope()
        self.generic_visit(node)
        self._exit_scope()

    def visit_Name(self, node: ast.Name) -> None:
        if isinstance(node.ctx, ast.Load):
            var_name = node.id
            for scope in reversed(self.scopes):
                if var_name in scope['defs']:
                    scope['used'].add(var_name)
                    break
        self.generic_visit(node)

    def visit_Assign(self, node: ast.Assign) -> None:
        for target in node.targets:
            self._handle_assignment_target(target)
        self.generic_visit(node)

    def visit_For(self, node: ast.For) -> None:
        self._handle_assignment_target(node.target)
        self.generic_visit(node)

    def visit_AsyncFor(self, node: ast.AsyncFor) -> None:
        self._handle_assignment_target(node.target)
        self.generic_visit(node)

    def visit_With(self, node: ast.With) -> None:
        for item in node.items:
            if item.optional_vars:
                self._handle_assignment_target(item.optional_vars)
        self.generic_visit(node)

    def visit_ExceptHandler(self, node: ast.ExceptHandler) -> None:
        if node.name and not self._is_ignored(node.name):
            self._record_variable(node.name, node.lineno)
        self.generic_visit(node)

    def visit_comprehension(self, node: ast.comprehension) -> None:
        self._handle_assignment_target(node.target)
        self.generic_visit(node)

    def visit_ListComp(self, node: ast.ListComp) -> None:
        self._enter_scope()
        for gen in node.generators:
            self.visit(gen)
        self.generic_visit(node)
        self._exit_scope()

    def visit_SetComp(self, node: ast.SetComp) -> None:
        self._enter_scope()
        for gen in node.generators:
            self.visit(gen)
        self.generic_visit(node)
        self._exit_scope()

    def visit_DictComp(self, node: ast.DictComp) -> None:
        self._enter_scope()
        for gen in node.generators:
            self.visit(gen)
        self.generic_visit(node)
        self._exit_scope()

    def visit_GeneratorExp(self, node: ast.GeneratorExp) -> None:
        self._enter_scope()
        for gen in node.generators:
            self.visit(gen)
        self.generic_visit(node)
        self._exit_scope()

    def check(self, tree: ast.AST) -> list[LintError]:
        self.errors.clear()
        self.scopes = []
        self._enter_scope()
        self.visit(tree)
        self._exit_scope()
        return self.errors


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
            UnusedVariablesRule(rule_name="unused_variable"),
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
