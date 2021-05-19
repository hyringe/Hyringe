package hycallparser.visitors_and_listeners;

import hycallparser.antlr.HycallBaseListener;
import hycallparser.antlr.HycallParser;
import hycallparser.builtinProcedures.Procedure;

public class ProgramStructureListener extends HycallBaseListener {

    private ProgramState program;

    public ProgramStructureListener() {
        program = new ProgramState();
    }

    public ProgramState  getProgramState() {
        return program;
    }

    @Override
    public void enterProgram(HycallParser.ProgramContext ctx) {
        ctx.globalvar_declaration().forEach(g -> g.enterRule(this));
        ctx.procedure().forEach(p -> p.enterRule(this));
    }

    @Override
    public void enterGlobalvar_noval(HycallParser.Globalvar_novalContext ctx) {
        ctx.IDENTIFIER().stream().forEach(gv -> program.addNewGlobalVar(gv.getText()));
    }

    @Override
    public void enterGlobalvar_num(HycallParser.Globalvar_numContext ctx) {
        program.addNewGlobalVar(ctx.name.getText(), new NumberVisitor().visit(ctx.number()));
    }

    @Override
    public void enterProcedure(HycallParser.ProcedureContext ctx) {
        program.addProcedure(ctx);
    }
}
