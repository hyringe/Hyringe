package hycallparser.builtinProcedures;

import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterInteger;

import java.math.BigInteger;
import java.util.List;
import java.util.Random;

public class ProcedureRandExp implements Procedure {

    @Override
    public String getName() {
        return "exp";
    }

    @Override
    public int getParameterCount() {
        return 1;
    }

    @Override
    public Parameter perform(List<Parameter> args) {
        Parameter scale = args.get(0);

        if (!scale.isInteger())
            throw new IllegalArgumentException("The \"rand\" procedure expects one integer as an argument!");

        return new ParameterInteger(BigInteger.valueOf((long) Math.log(1-new Random().nextDouble())*-scale.getInt().intValue()));
    }
}
