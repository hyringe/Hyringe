package hycallparser.visitors_and_listeners;

import hycallparser.antlr.HycallBaseListener;
import hycallparser.antlr.HycallParser;
import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterNone;

import java.util.Map;

public class ProcedureExecutor extends HycallBaseListener {

    private ProgramState state;
    private ProcedureState ps;
    private Parameter lastExpressionResult = ParameterNone.instance();

    public ProcedureExecutor(ProgramState state) {
        this.state = state;
        ps = new ProcedureState();
    }

    public ProcedureExecutor(ProgramState state, Map<String, Parameter> parameterValues) {
        this.state = state;
        ps = new ProcedureState(parameterValues);
    }

    @Override
    public void enterProcedure(HycallParser.ProcedureContext ctx) {
        ctx.statement().forEach(s -> s.enterRule(this));
    }


    @Override
    public void enterStatement_FOR(HycallParser.Statement_FORContext ctx) {
        Parameter vals = new ExpressionEvaluator(state, ps).visit(ctx.expression());
        if (!vals.isList())
            throw new IllegalArgumentException("A for-loop can only iterate through a list!");

        for (Parameter p : vals.getValues()) {
            ps.setVariable(ctx.IDENTIFIER().getText(), p);
            ctx.statement().enterRule(this);
        }
    }

    @Override
    public void enterStatement_BRACE(HycallParser.Statement_BRACEContext ctx) {
        ctx.statement().forEach(s -> s.enterRule(this));
    }

    @Override
    public void enterStatement_EXPRESSION(HycallParser.Statement_EXPRESSIONContext ctx) {
        lastExpressionResult = new ExpressionEvaluator(state, ps).visit(ctx.expression());
    }

    public Parameter lastExpressionResult() {
        return lastExpressionResult;
    }
}