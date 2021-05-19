package hycallparser.builtinProcedures;

import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterInteger;
import hycallparser.parameter.ParameterList;

import java.math.BigInteger;
import java.util.List;

public class ProcedureIntegerBounds implements Procedure {

    @Override
    public String getName() {
        return "integerBounds";
    }

    @Override
    public int getParameterCount() {
        return 1;
    }

    @Override
    public Parameter perform(List<Parameter> args) {
        Parameter bitcount = args.get(0);

        if (!bitcount.isInteger())
            throw new IllegalArgumentException("The \"integerBounds\" procedure expects one integer as an argument!");

        Parameter zero = new ParameterInteger(BigInteger.ZERO);
        Parameter one = new ParameterInteger(BigInteger.ONE);
        Parameter smax = new ProcedureSignedMax().perform(args);
        Parameter umax = new ProcedureUnsignedMax().perform(args);
        return new ParameterList(List.of(zero, one, smax, umax));
    }
}
