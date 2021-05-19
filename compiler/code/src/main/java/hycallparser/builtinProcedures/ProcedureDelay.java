package hycallparser.builtinProcedures;

import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterNone;
import hycallparser.visitors_and_listeners.campaignListeners.CampaignListenerManager;

import java.util.List;

public class ProcedureDelay implements Procedure {

    @Override
    public String getName() {
        return "delay";
    }

    @Override
    public int getParameterCount() {
        return 1;
    }

    @Override
    public Parameter perform(List<Parameter> args) {
        Parameter time = args.get(0);

        if (!time.isInteger())
            throw new IllegalArgumentException("The \"delay\" procedure expects one integer as an argument!");

        CampaignListenerManager.getActiveCampaignListener().encodeDelay(time.getInt().longValue());

        return ParameterNone.instance();
    }
}
