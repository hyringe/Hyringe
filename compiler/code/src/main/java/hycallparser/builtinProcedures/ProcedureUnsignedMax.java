package hycallparser.builtinProcedures;

import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterInteger;

import java.math.BigInteger;
import java.util.List;

public class ProcedureUnsignedMax implements Procedure {

    @Override
    public String getName() {
        return "unsignedMax";
    }

    @Override
    public int getParameterCount() {
        return 1;
    }

    @Override
    public Parameter perform(List<Parameter> args) {
        Parameter bitcount = args.get(0);

        if (!bitcount.isInteger())
            throw new IllegalArgumentException("The \"unsignedMax\" procedure expects one integer as an argument!");

        return new ParameterInteger(BigInteger.valueOf(2).pow(bitcount.getInt().intValue()).subtract(BigInteger.valueOf(1)));
    }
}
