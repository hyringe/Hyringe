package hycallparser.builtinProcedures;

import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterInteger;

import java.math.BigInteger;
import java.util.List;
import java.util.Random;

public class ProcedureRandomUniform implements Procedure {
    @Override
    public String getName() {
        return "rand";
    }

    @Override
    public int getParameterCount() {
        return 1;
    }

    @Override
    public Parameter perform(List<Parameter> args) {
        Parameter bitcount = args.get(0);

        if (!bitcount.isInteger())
            throw new IllegalArgumentException("The \"rand\" procedure expects one integer as an argument!");

        return new ParameterInteger(new BigInteger(bitcount.getInt().intValue(), new Random()));
    }
}
