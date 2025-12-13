# 1 "./src/managers/SnippetManager.cpp"
#include "SnippetManager.h"

SnippetManager::SnippetManager() { loadDefaultSnippets(); }

void SnippetManager::addSnippet(const CodeSnippet& snippet) { m_snippets[snippet.name] = snippet; }

void SnippetManager::removeSnippet(const QString& name) { m_snippets.remove(name); }

CodeSnippet SnippetManager::getSnippet(const QString& name) const { return m_snippets.value(name); }

QList<CodeSnippet> SnippetManager::getSnippetsByLanguage(const QString& language) const {
    QList<CodeSnippet> result;
    for (const auto& snippet : m_snippets) {
        if (snippet.language == language) {
            result.append(snippet);
        }
    }
    return result;
}

QStringList SnippetManager::getAllSnippetNames() const { return m_snippets.keys(); }

void SnippetManager::loadDefaultSnippets() {
    addSnippet({"Hello World C++", "A simple Hello World program in C++",
                "#include <iostream>\n\nint main() {\n    std::cout << \"Hello, World!\" << "
                "std::endl;\n    return 0;\n}",
                "C++"});

    addSnippet({"Vector Operations C++", "Basic vector operations in C++",
                "#include <iostream>\n#include <vector>\n\nint main() {\n    std::vector<int> v = "
                "{1, 2, 3, 4, 5};\n    for (int num : v) {\n        std::cout << num << \" \";\n   "
                " }\n    return 0;\n}",
                "C++"});

    addSnippet({"Class Definition C++", "A simple class definition",
                "class MyClass {\npublic:\n    MyClass() { }\n    void method() { }\nprivate:\n    "
                "int m_value;\n};",
                "C++"});

    addSnippet({"Hello World Java", "A simple Hello World program in Java",
                "public class HelloWorld {\n    public static void main(String[] args) {\n        "
                "System.out.println(\"Hello, World!\");\n    }\n}",
                "Java"});

    addSnippet({"ArrayList Java", "ArrayList example in Java",
                "import java.util.ArrayList;\n\npublic class Main {\n    public static void "
                "main(String[] args) {\n        ArrayList<String> list = new ArrayList<>();\n      "
                "  list.add(\"item1\");\n        for (String item : list) {\n            "
                "System.out.println(item);\n        }\n    }\n}",
                "Java"});

    addSnippet({"Hello World Python", "A simple Hello World program in Python",
                "print('Hello, World!')", "Python"});

    addSnippet({"List Comprehension", "Python list comprehension example",
                "numbers = [1, 2, 3, 4, 5]\nsquared = [x**2 for x in numbers]\nprint(squared)",
                "Python"});

    addSnippet({"Function Definition", "Python function definition",
                "def greet(name):\n    return f'Hello, {name}!'\n\nprint(greet('World'))",
                "Python"});

    addSnippet({"File Operations", "Python file read/write",
                "# Write to file\nwith open('file.txt', 'w') as f:\n    f.write('Hello, "
                "World!')\n\n# Read from file\nwith open('file.txt', 'r') as f:\n    content = "
                "f.read()\n    print(content)",
                "Python"});

    addSnippet({"Hello World C#", "A simple Hello World program in C#",
                "using System;\n\nclass Program {\n    static void Main() {\n        "
                "Console.WriteLine(\"Hello, World!\");\n    }\n}",
                "C#"});

    addSnippet({"LINQ Query C#", "LINQ query example in C#",
                "using System;\nusing System.Linq;\n\nclass Program {\n    static void Main() {\n  "
                "      int[] numbers = { 1, 2, 3, 4, 5 };\n        var evens = numbers.Where(n => "
                "n % 2 == 0);\n        foreach (var n in evens) Console.WriteLine(n);\n    }\n}",
                "C#"});

    addSnippet({"Hello World Rust", "A simple Hello World program in Rust",
                "fn main() {\n    println!(\"Hello, World!\");\n}", "Rust"});

    addSnippet({"Vector Rust", "Vector operations in Rust",
                "fn main() {\n    let mut v = vec![1, 2, 3, 4, 5];\n    v.push(6);\n    for num in "
                "&v {\n        println!(\"{}\", num);\n    }\n}",
                "Rust"});

    addSnippet({"Hello World Ada", "A simple Hello World program in Ada",
                "with Ada.Text_IO; use Ada.Text_IO;\n\nprocedure Hello is\nbegin\n   "
                "Put_Line(\"Hello, World!\");\nend Hello;",
                "Ada"});

    addSnippet({"Fibonacci Ada", "Fibonacci function in Ada",
                "with Ada.Text_IO; use Ada.Text_IO;\n\nprocedure Fibonacci is\n   function Fib(N : "
                "Natural) return Natural is\n   begin\n      if N <= 1 then\n         return N;\n  "
                "    else\n         return Fib(N - 1) + Fib(N - 2);\n      end if;\n   end Fib;\n"
                "begin\n   Put_Line(\"Fib(10) = \" & Natural'Image(Fib(10)));\nend Fibonacci;",
                "Ada"});

    addSnippet({"Array Operations Ada", "Array operations in Ada",
                "with Ada.Text_IO; use Ada.Text_IO;\n\nprocedure Arrays is\n   type Int_Array is "
                "array (1 .. 5) of Integer;\n   Numbers : Int_Array := (1, 2, 3, 4, 5);\nbegin\n   "
                "for I in Numbers'Range loop\n      Put_Line(Integer'Image(Numbers(I)));\n   end "
                "loop;\nend Arrays;",
                "Ada"});

    addSnippet(
        {"Record Type Ada", "Record type definition in Ada",
         "procedure Records is\n   type Person is record\n      Name : String(1 .. 20);\n  "
         "    Age  : Natural;\n   end record;\n\n   P : Person := (Name => \"John Doe         "
         "   \", Age => 30);\nbegin\n   null;\nend Records;",
         "Ada"});
}
