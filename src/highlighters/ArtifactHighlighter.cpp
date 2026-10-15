#include "ArtifactHighlighter.h"

ArtifactHighlighter::ArtifactHighlighter(QTextDocument* parent, ArtifactType type)
    : QSyntaxHighlighter(parent), m_type(type) {
    buildRules();
}

void ArtifactHighlighter::addRule(const QString& pattern, SyntaxRole role, bool bold, bool italic, int group,
                                  QRegularExpression::PatternOptions opts) {
    Rule r;
    r.pattern = QRegularExpression(pattern, opts);
    r.role = role;
    r.bold = bold;
    r.italic = italic;
    r.captureGroup = group;
    m_rules.append(r);
}

void ArtifactHighlighter::buildRules() {
    switch (m_type) {
        case ArtifactType::LLVM_IR:
            addRule(R"(\b(define|declare|global|constant|private|internal|external|linkonce_odr|dso_local|local_unnamed_addr|unnamed_addr|nounwind|noinline|optnone|attributes|target|datalayout|triple|source_filename|align|inbounds|nsw|nuw|volatile|tail|to)\b)", SyntaxRole::Preprocessor);
            addRule(R"(\b(ret|br|switch|unreachable|invoke|add|sub|mul|sdiv|udiv|srem|urem|fadd|fsub|fmul|fdiv|shl|lshr|ashr|and|or|xor|alloca|load|store|getelementptr|trunc|zext|sext|fptrunc|fpext|fptoui|fptosi|uitofp|sitofp|ptrtoint|inttoptr|bitcast|icmp|fcmp|phi|select|call|extractvalue|insertvalue)\b)", SyntaxRole::Keyword, true);
            addRule(R"(\b(void|half|float|double|ptr|label|metadata|i\d+)\b)", SyntaxRole::Type);
            addRule(R"(@[-a-zA-Z$._0-9"]+)", SyntaxRole::Function);
            addRule(R"(%[-a-zA-Z$._0-9"]+)", SyntaxRole::Register);
            addRule(R"(\b(eq|ne|ugt|uge|ult|ule|sgt|sge|slt|sle|oeq|one|ogt|oge|olt|ole)\b)", SyntaxRole::Label);
            addRule(R"(\b-?\d+(\.\d+)?\b)", SyntaxRole::Number);
            addRule(R"(^[-a-zA-Z$._0-9]+:)", SyntaxRole::Label, true);
            addRule(R"("[^"]*")", SyntaxRole::String);
            addRule(R"(;.*$)", SyntaxRole::Comment, false, true);
            break;
        case ArtifactType::JVM_Bytecode:
            addRule(R"(\b(aload|astore|iload|istore|lload|lstore|fload|fstore|dload|dstore)(_\d)?\b)", SyntaxRole::Register);
            addRule(R"(\b(invokespecial|invokevirtual|invokestatic|invokeinterface|invokedynamic|new|newarray|anewarray|getfield|putfield|getstatic|putstatic|checkcast|instanceof|athrow)\b)", SyntaxRole::Function, true);
            addRule(R"(\b(return|ireturn|lreturn|freturn|dreturn|areturn|goto|ifeq|ifne|iflt|ifge|ifgt|ifle|if_icmp\w+|if_acmp\w+|ifnull|ifnonnull|tableswitch|lookupswitch)\b)", SyntaxRole::Keyword, true);
            addRule(R"(\b(iconst_m1|iconst_\d|lconst_\d|fconst_\d|dconst_\d|bipush|sipush|ldc2_w|ldc_w|ldc|aconst_null|dup\w*|pop2?|swap|nop|iadd|isub|imul|idiv|irem|ineg|ladd|lsub|lmul|ldiv|fadd|dadd|iinc|arraylength|[ailfd]aload|[ailfd]astore)\b)", SyntaxRole::Keyword);
            addRule(R"(#\d+)", SyntaxRole::Number);
            addRule(R"(^\s*(Code:|LineNumberTable:|LocalVariableTable:|Exception table:|Constant pool:|SourceFile:|InnerClasses:|BootstrapMethods:))", SyntaxRole::Preprocessor, true);
            addRule(R"(^\s*\d+:)", SyntaxRole::Label);
            addRule(R"(\b(public|private|protected|static|final|abstract|class|interface|synchronized|native|enum)\b)", SyntaxRole::Type);
            addRule(R"(//.*$)", SyntaxRole::Comment, false, true);
            break;
        case ArtifactType::CPython_Bytecode:
            addRule(R"(\b[A-Z][A-Z0-9_]{2,}\b)", SyntaxRole::Keyword, true);
            addRule(R"(^\s*\d+\s)", SyntaxRole::Label);
            addRule(R"(\b\d+\b)", SyntaxRole::Number);
            addRule(R"(\([^)]*\))", SyntaxRole::String);
            addRule(R"(^\s*>>\s*\d+|\bDisassembly of .*$)", SyntaxRole::Preprocessor, true);
            addRule(R"(^;.*$)", SyntaxRole::Comment, false, true);
            break;
        case ArtifactType::CIL:
            addRule(R"(\.\w+)", SyntaxRole::Preprocessor);
            addRule(R"(\b(ldarg|ldloc|stloc|ldc|ldstr|call|callvirt|newobj|ret|br|brtrue|brfalse|add|sub|mul|div|nop|pop|dup|ldfld|stfld|box|unbox)\w*(\.\w+)*\b)", SyntaxRole::Keyword, true);
            addRule(R"(\bIL_[0-9a-fA-F]{4}:)", SyntaxRole::Label);
            addRule(R"(//.*$)", SyntaxRole::Comment, false, true);
            break;
        default:
            break;
    }
}

void ArtifactHighlighter::applyTheme() {
    m_themeGeneration = Theme::generation();
    rehighlight();
}

void ArtifactHighlighter::highlightBlock(const QString& text) {
    m_themeGeneration = Theme::generation();
    for (const Rule& r : std::as_const(m_rules)) {
        QTextCharFormat fmt;
        fmt.setForeground(Theme::syntaxColor(r.role));
        if (r.bold) fmt.setFontWeight(QFont::Bold);
        if (r.italic) fmt.setFontItalic(true);
        auto it = r.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto m = it.next();
            if (m.capturedLength(r.captureGroup) > 0)
                setFormat(m.capturedStart(r.captureGroup), m.capturedLength(r.captureGroup), fmt);
        }
    }
}
