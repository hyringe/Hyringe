package hycallparser.visitors_and_listeners;

import hycallparser.antlr.HycallBaseVisitor;
import hycallparser.antlr.HycallParser;
import hycallparser.builtinProcedures.Procedure;
import hycallparser.builtinProcedures.ProcedureManager;
import hycallparser.parameter.*;

import java.math.BigInteger;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.stream.IntStream;
import java.util.stream.Stream;

public class ExpressionEvaluator extends HycallBaseVisitor<Parameter> {

    private ProgramState progState;
    private ProcedureState procState;

    public ExpressionEvaluator(ProgramState progState, ProcedureState procState) {
        this.progState = progState;
        this.procState = procState;
    }

    @Override
    public Parameter visitExpression_MULDIV(HycallParser.Expression_MULDIVContext ctx) {
        Parameter left = this.visit(ctx.expression(0));
        Parameter right = this.visit(ctx.expression(1));

        if (!(left.isInteger() && right.isInteger()))
            throw new IllegalArgumentException("Operator \"" + ctx.op + "\" can only be applied to integers");

        switch (ctx.op.getText()) {
            case "*":
                return new ParameterInteger(left.getInt().multiply(right.getInt()));
            case "/":
                return new ParameterInteger(left.getInt().divide(right.getInt()));
            case "%":
                return new ParameterInteger(left.getInt().mod(right.getInt()));
        }

        return null;
    }

    @Override
    public Parameter visitExpression_NUM(HycallParser.Expression_NUMContext ctx) {
        return new ParameterInteger(new NumberVisitor().visit(ctx.number()));
    }

    @Override
    public Parameter visitExpression_ASSIGN(HycallParser.Expression_ASSIGNContext ctx) {
        String varname = ctx.IDENTIFIER().getText();
        Parameter val = this.visit(ctx.expression());

        if (progState.existGlobalVar(varname))
            progState.setGlobalVariable(varname, val);
        else
            procState.setVariable(varname, val);

        return val;
    }

    @Override
    public Parameter visitExpression_SIGN(HycallParser.Expression_SIGNContext ctx) {
        Parameter val = this.visit(ctx.expression());
        if (!val.isInteger())
            throw new IllegalArgumentException("Unary operator \"" + ctx.op + "\" can only be applied to integers");

        return ctx.op.equals("+") ? val : new ParameterInteger(val.getInt().multiply(BigInteger.valueOf(-1)));
    }

    @Override
    public Parameter visitExpression_STR(HycallParser.Expression_STRContext ctx) {
        return new ParameterString(ctx.getText().substring(1, ctx.getText().length()-1));
    }

    @Override
    public Parameter visitExpression_PARENS(HycallParser.Expression_PARENSContext ctx) {
        return this.visit(ctx.expression());
    }

    @Override
    public Parameter visitExpression_VAR(HycallParser.Expression_VARContext ctx) {
        String varname = ctx.getText();
        if (progState.existGlobalVar(varname))
            return progState.getGlobalVar(varname);
        if (procState.existVar(varname))
            return procState.getVar(varname);
        throw new IllegalArgumentException("Variable \"" + varname + "\" is used, but not defined!");
    }

    @Override
    public Parameter visitExpression_ADDSUB(HycallParser.Expression_ADDSUBContext ctx) {
        Parameter left = this.visit(ctx.expression(0));
        Parameter right = this.visit(ctx.expression(1));

        if (ctx.op.equals("-")) {
            if (!(left.isInteger() && right.isInteger()))
                throw new IllegalArgumentException("Operator \"-\" can only be applied to integers!");
            return new ParameterInteger(left.getInt().subtract(right.getInt()));
        }

        // ctx.op.equals("+")
        if (left.isInteger() && right.isInteger())
            return new ParameterInteger(left.getInt().add(right.getInt()));
        if (left.isString() && right.isString())
            return new ParameterString(left.getString() + right.getString());
        if (left.isList() && right.isList())
            return new ParameterList(Stream.of(left.getValues(), right.getValues()).flatMap(List::stream).collect(Collectors.toList()));

        throw new IllegalArgumentException("Operator \"+\" can only be applied to either Integers, Strings or Lists");
    }

    @Override
    public Parameter visitExpression_KEYVAL(HycallParser.Expression_KEYVALContext ctx) {
        Parameter key = this.visit(ctx.expression(0));
        if (!key.isString())
            throw new IllegalArgumentException("The key of a key-value pair has to be a string");
        Parameter val = this.visit(ctx.expression(1));
        return new ParameterKeyValue(key.getString(), val);
    }

    @Override
    public Parameter visitExpression_LIST(HycallParser.Expression_LISTContext ctx) {
        return new ParameterList(ctx.expression().stream().map(this::visit).collect(Collectors.toList()));
    }

    @Override
    public Parameter visitExpression_KEYACCESS(HycallParser.Expression_KEYACCESSContext ctx) {
        Parameter kvp = this.visit(ctx.expression());
        if (!kvp.isKeyValue())
            throw new IllegalArgumentException("Operator \"." + ctx.op.getText() + "\" can only be applied to a key-value pair!");
        return ctx.op.getText().equals("key") ? new ParameterString(kvp.getKey()) : kvp.getValue();
    }

    @Override
    public Parameter visitExpression_LISTACCESS(HycallParser.Expression_LISTACCESSContext ctx) {
        Parameter list = this.visit(ctx.list);
        Parameter index = this.visit(ctx.index);

        if (!list.isList())
            throw new IllegalArgumentException("Operator [] can only be applied to lists!");

        if (index.isInteger()) {
            int i = index.getInt().intValue();
            try {
                return list.getValues().get(i);
            } catch (Exception e) {
                throw new IllegalArgumentException("Out of bounds: Index " + i + " does not exist!");
            }
        }
        else if (index.isString())
            return ((ParameterList) list).findKVPwithKey(index.getString()).map(Parameter::getValue).orElseThrow(() -> new IllegalArgumentException("No key-value pair with key \"" + index + "\" found;"));
        else
            throw new IllegalArgumentException("Only integers and string can be used as indices in the [] operator!");
    }

    @Override
    public Parameter visitExpression_PROCCALL(HycallParser.Expression_PROCCALLContext ctx) {
        String procname = ctx.IDENTIFIER().getText();
        List<Parameter> parameterValues = ProcedureUtils.getParameterValues(ctx, this);

        // check if it is a builtin procedure
        Optional<Procedure> builtin = ProcedureManager.getProcedure(procname, parameterValues.size());
        if (builtin.isPresent())
            return builtin.get().perform(parameterValues);

        // check if it is a user defined procedure
        HycallParser.ProcedureContext pc = progState.getProcedure(procname);
        List<String> parameterNames = ProcedureUtils.getParameters(pc);

        if (parameterNames.size() != parameterValues.size())
            throw new IllegalArgumentException("Procedure \"" + procname + "\" expects " + parameterNames.size() + " parameter(s), but " + parameterValues.size() + " was/were supplied!");

        Map<String, Parameter> parameterMapping = IntStream.range(0, parameterNames.size()).boxed().collect(Collectors.toMap(i -> parameterNames.get(i), i -> parameterValues.get(i)));
        ProcedureExecutor pe = new ProcedureExecutor(progState, parameterMapping);
        pc.enterRule(pe);
        return pe.lastExpressionResult();
    }
}