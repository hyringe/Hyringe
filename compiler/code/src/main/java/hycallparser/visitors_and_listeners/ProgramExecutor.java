package hycallparser.visitors_and_listeners;

import hycallparser.antlr.HycallParser;
import hycallparser.visitors_and_listeners.campaignListeners.CampaignListener;

public class ProgramExecutor {

    private ProgramState state;

    public ProgramExecutor(ProgramState state) {
        this.state = state;
    }

    public void execute() {
        if (state.existsProcedure("init")) {
            HycallParser.ProcedureContext init_pc = state.getProcedure("init");
            if (!ProcedureUtils.getParameters(init_pc).isEmpty())
                throw new IllegalArgumentException("The \"init\" procedure cannot have parameters");
            init_pc.enterRule(new ProcedureExecutor(state));
        }

        if (!state.existsProcedure("main"))
            throw new IllegalArgumentException("Procedure \"main\" not found!");
        HycallParser.ProcedureContext main_pc = state.getProcedure("main");
        if (!ProcedureUtils.getParameters(main_pc).isEmpty())
            throw new IllegalArgumentException("The \"main\" procedure cannot have parameters");
        main_pc.enterRule(new ProcedureExecutor(state));
    }
}
