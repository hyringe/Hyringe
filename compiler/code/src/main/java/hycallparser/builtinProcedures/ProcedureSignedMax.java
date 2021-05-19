package hycallparser.builtinProcedures;

import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterInteger;

import java.math.BigInteger;
import java.util.List;

public class ProcedureSignedMax implements Procedure {

    @Override
    public String getName() {
        return "signedMax";
    }

    @Override
    public int getParameterCount() {
        return 1;
    }

    @Override
    public Parameter perform(List<Parameter> args) {
        Parameter bitcount = args.get(0);

        if (!bitcount.isInteger())
            throw new IllegalArgumentException("The \"signedMax\" procedure expects one integer as an argument!");

        return new ParameterInteger(BigInteger.valueOf(2).pow(bitcount.getInt().intValue()-1).subtract(BigInteger.valueOf(1)));
    }
}
